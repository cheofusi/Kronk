#include "IRGenAide.h"
#include "Names.h"
#include "Nodes.h"


Type* BuiltinTyId::typegen() {
	if (builtinTypeId == "bool") {
		return Attr::Builder.getInt1Ty();
	}

	else if (builtinTypeId == "reel") {
		return Attr::Builder.getDoubleTy();
	}

	else if (builtinTypeId == "str") {
		return StructType::get(Attr::Context,
		                       { Attr::Builder.getInt64Ty(), Attr::Builder.getInt8PtrTy() });
	}


	// This will never happen as the parser cannot miss this.
	irGenAide::LogCodeGenError("Primitive Type not recognized");
}


Type* EntityTyId::typegen() { return names::EntityType(enttyTypeId).value(); }


Type* ListTyId::typegen() {
	auto lstElemTy = lstTypeId->typegen();
	// The first element of a liste entity is the its size. The second is a POINTER TO ITS ELEMENT TYPE.

	if (lstElemTy->isStructTy()) {
		// the elements (entities) in this case are also pointers
		// ex. liste(liste(nombre)) -> { i64, { i64, double* }** }, liste(Point) -> { i64, Point** }
		return StructType::get(
		    Attr::Context, { Attr::Builder.getInt64Ty(), lstElemTy->getPointerTo()->getPointerTo() });
	}
	// ex. liste(nombre) -> { i64, double* }
	return StructType::get(Attr::Context, { Attr::Builder.getInt64Ty(), lstElemTy->getPointerTo() });
}