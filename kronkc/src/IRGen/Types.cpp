#include "Nodes.h"
#include "irGenAide.h"

Type* BuiltinTyId::typegen() {
    if (prityId == "bool") {
        return Attr::Builder.getInt1Ty();
    }

    else if (prityId == "reel") {
        return Attr::Builder.getDoubleTy();
    }

    else if(prityId == "str") {
        return StructType::get(Attr::Context, { Attr::Builder.getInt64Ty(), Attr::Builder.getInt8PtrTy() });
    }


    // This will never happen as the parser cannot miss this.
    irGenAide::LogCodeGenError("Primitive Type not recognized");
}


Type* EntityTyId::typegen() {
    return Attr::EntityTypes[enttyId];
}


Type* ListTyId::typegen() {
    auto lstElemTy = lsttyId->typegen();
    // The first element of a liste entity is the its size. The second is a POINTER TO ITS ELEMENT TYPE.

    if(lstElemTy->isStructTy()) {
        // the elements (entities) in this case are also pointers
        // ex. liste(liste(nombre)) -> { i64, { i64, double* }** }, liste(Point) -> { i64, Point** }
        return StructType::get(Attr::Context, { Attr::Builder.getInt64Ty(), lstElemTy->getPointerTo()->getPointerTo() });
    }
    // ex. liste(nombre) -> { i64, double* }
    return StructType::get(Attr::Context, { Attr::Builder.getInt64Ty(), lstElemTy->getPointerTo() });
}