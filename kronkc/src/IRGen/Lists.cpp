#include "IRGenAide.h"
#include "Nodes.h"


Value* AnonymousList::codegen() {
	LogProgress("Creating anonymous list");

	std::vector<Value*> initListV;
	Value* prevInitElementV;
	unsigned int idx = 0;
	for (auto& initElement : initList) {
		// first check if element is of the same type as the previous element. This is to ensure the
		// validity of all elements before allocation memory and filling the symbol table.
		auto initElementV = initElement->codegen();
		if (idx >= 1) {
			if (not types::isEqual(initElementV->getType(), prevInitElementV->getType())) {
				irGenAide::LogCodeGenError("Element #" + std::to_string(idx + 1) + " in " +
				                           "initializer list different from previous elements");
			}
		}

		initListV.push_back(initElementV);
		prevInitElementV = initElementV;
		idx++;
	}

	auto sizeV = irGenAide::getConstantInt(initListV.size());
	auto listElementType = initListV[0]->getType();

	AllocaInst* dataPtr = Attr::Builder.CreateAlloca(listElementType, sizeV);
	AllocaInst* lstPtr =
	    Attr::Builder.CreateAlloca(StructType::get(Attr::Builder.getInt64Ty(), dataPtr->getType()));

	irGenAide::fillUpListEntty(lstPtr, { sizeV, dataPtr }, initListV);
	Attr::ScopeStack.back()->HeapAllocas.push_back(lstPtr);

	return lstPtr;
}


Value* AnonymousString::codegen() {
	LogProgress("Creating anonymous string");

	std::vector<Value*> strV;
	for (auto& strchar : str) {
		strV.push_back(Attr::Builder.getInt8(strchar));
	}

	auto sizeV = irGenAide::getConstantInt(strV.size());

	AllocaInst* dataPtr = Attr::Builder.CreateAlloca(Attr::Builder.getInt8Ty(), sizeV);
	AllocaInst* lstPtr =
	    Attr::Builder.CreateAlloca(StructType::get(Attr::Builder.getInt64Ty(), dataPtr->getType()));

	irGenAide::fillUpListEntty(lstPtr, { sizeV, dataPtr }, strV);
	Attr::ScopeStack.back()->HeapAllocas.push_back(lstPtr);

	return lstPtr;
}


Value* ListConcatenation::codegen() {
	// BinaryExpr already checked if list1 & list2 are both listPtrs. So we don't need to
	// now the magic begins
	auto Lsize = Attr::Builder.CreateLoad(irGenAide::getGEPAt(list1, irGenAide::getConstantInt(0)));
	auto LdataPtr = Attr::Builder.CreateLoad(irGenAide::getGEPAt(list1, irGenAide::getConstantInt(1)));
	auto Rsize = Attr::Builder.CreateLoad(irGenAide::getGEPAt(list2, irGenAide::getConstantInt(0)));
	auto RdataPtr = Attr::Builder.CreateLoad(irGenAide::getGEPAt(list2, irGenAide::getConstantInt(1)));

	// check whether the data types are equal
	if (not types::isEqual(LdataPtr->getType(), RdataPtr->getType()))
		irGenAide::LogCodeGenError("Trying to concatenate lists with unequal types");

	auto newListSizeV = Attr::Builder.CreateAdd(Lsize, Rsize);
	// The starting address of of the underlying memory block
	AllocaInst* newDataPtr =
	    Attr::Builder.CreateAlloca(LdataPtr->getType()->getPointerElementType(), newListSizeV);

	// we now get all data from both listes in order
	irGenAide::emitMemcpy(newDataPtr, LdataPtr, Lsize);
	irGenAide::emitMemcpy(irGenAide::getGEPAt(newDataPtr, Lsize, true), RdataPtr, Rsize);

	AllocaInst* newlistPtr =
	    Attr::Builder.CreateAlloca(StructType::get(Attr::Builder.getInt64Ty(), newDataPtr->getType()));

	irGenAide::fillUpListEntty(newlistPtr, { newListSizeV, newDataPtr });

	return newlistPtr;
}


Value* ListIdxRef::codegen() {
	auto lstPtr = list->codegen();
	if (not types::isListePtr(lstPtr)) {
		irGenAide::LogCodeGenError(
		    "Trying to perform a list operation on a value that is not a liste !!");
	}

	auto listSizeV = Attr::Builder.CreateLoad(irGenAide::getGEPAt(lstPtr, irGenAide::getConstantInt(0)));

	auto idxV = idx->codegen();
	// cast double to int (since all the nombre type in kronk is a double)
	idxV = irGenAide::DoubletoIntCast(idxV);

	// we insert a runtime check to see if the index is in the list bounds
	irGenAide::emitRtCheck("_kronk_list_idx_check", { idxV, listSizeV });

	// take care of negative indices
	idxV = irGenAide::emitRtCompilerUtilCall("_kronk_list_fix_idx", { idxV, listSizeV });

	auto listDataPtr =
	    Attr::Builder.CreateLoad(irGenAide::getGEPAt(lstPtr, irGenAide::getConstantInt(1)));
	auto ptrToDataAtIdx = irGenAide::getGEPAt(listDataPtr, idxV, true);

	if (listDataPtr->getType()->getPointerElementType()->isIntegerTy(8)) {
		// this data pointer is an i8*. Or a string. We won't load the i8* ptrToDataAtIdx and return an
		// i8 nor return the i8*. Because in kronk, they're no characters. There are just strings i.e {
		// i64, i8* }. So instead we'll construct a new { i64, i8*} and fill it with 1 & ptrToDataAtIdx.
		// Now on doing something like
		//                       soit name = "joe"
		//                       soit str = name[-1]
		// str's value will be "e", a string in itself of size 1. Hence str is initialized as a string.

		if (ctx) {
			// we're not modifying the string
			auto sizeV = irGenAide::getConstantInt(1);
			AllocaInst* dataPtr = Attr::Builder.CreateAlloca(Attr::Builder.getInt8Ty(), sizeV);
			AllocaInst* lstPtr = Attr::Builder.CreateAlloca(
			    StructType::get(Attr::Builder.getInt64Ty(), listDataPtr->getType()));

			auto indexedChar = Attr::Builder.CreateLoad(ptrToDataAtIdx);

			irGenAide::fillUpListEntty(lstPtr, { sizeV, dataPtr }, { indexedChar });
			Attr::ScopeStack.back()->HeapAllocas.push_back(lstPtr);

			return lstPtr;
		}

		// we're modifying the character at idxV of the string
		return ptrToDataAtIdx;
	}

	return (ctx) ? Attr::Builder.CreateLoad(ptrToDataAtIdx) : ptrToDataAtIdx;
}

bool ListIdxRef::injectCtx(bool ctx) {
	this->ctx = ctx;
	return true;
}


Value* ListSlice::codegen() {
	auto lstPtr = list->codegen();
	if (not types::isListePtr(lstPtr))
		irGenAide::LogCodeGenError(
		    "Trying to perform a list operation on a value that is not a liste !!");

	auto listSizeV = Attr::Builder.CreateLoad(irGenAide::getGEPAt(lstPtr, irGenAide::getConstantInt(0)));

	// first make sure start and end are valid indices given the list size
	auto startV = start ? start->codegen() : irGenAide::getConstantInt(0);
	auto endV = end ? end->codegen() : listSizeV;

	if (not startV->getType()->isIntegerTy(64)) {
		startV = irGenAide::DoubletoIntCast(startV);
	}

	if (not endV->getType()->isIntegerTy(64)) {
		endV = irGenAide::DoubletoIntCast(endV);
	}

	// we insert a runtime check to see if the slice is in the list bounds
	irGenAide::emitRtCheck("_kronk_list_slice_check", { startV, endV, listSizeV });

	// taking care of negative idx's
	startV = irGenAide::emitRtCompilerUtilCall("_kronk_list_fix_idx", { startV, listSizeV });
	endV = irGenAide::emitRtCompilerUtilCall("_kronk_list_fix_idx", { endV, listSizeV });

	auto spliceSizeV = Attr::Builder.CreateSub(endV, startV);
	auto oldDataPtr =
	    Attr::Builder.CreateLoad(irGenAide::getGEPAt(lstPtr, irGenAide::getConstantInt(1)));
	AllocaInst* newDataPtr =
	    Attr::Builder.CreateAlloca(oldDataPtr->getType()->getPointerElementType(), spliceSizeV);

	// transfer elements from original list to the splice
	irGenAide::emitMemcpy(newDataPtr, oldDataPtr, spliceSizeV);

	// build the list entity for the splice
	AllocaInst* newlstPtr =
	    Attr::Builder.CreateAlloca(StructType::get(Attr::Builder.getInt64Ty(), newDataPtr->getType()));
	irGenAide::fillUpListEntty(newlstPtr, std::vector<Value*>{ spliceSizeV, newDataPtr });

	return newlstPtr;
}
