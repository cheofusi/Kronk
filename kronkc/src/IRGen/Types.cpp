#include "Nodes.h"
#include "irGenAide.h"

Type* PrimitiveTyId::typegen() {
    if (prityId == "bool") {
        return builder.getInt1Ty();
    }
    else if (prityId == "reel") {
        return builder.getDoubleTy();
    }

    // This will never happen as the parser cannot miss this.
    irGenAide::LogCodeGenError("Primitive Type not recognized");
}


Type* EntityTyId::typegen() {
    return EntityTypes[enttyId];
}


Type* ListTyId::typegen() {
    auto lstElemTy = lsttyId->typegen();
    // The first element of a liste entity is the its size. The second is a POINTER TO ITS ELEMENT TYPE.

    if(lstElemTy->isStructTy()) {
        // the elements (entities) in this case are also pointers
        // ex. liste(liste(nombre)) -> { i64, { i64, double* }** }, liste(Point) -> { i64, Point** }
        return StructType::get(context, { builder.getInt64Ty(), lstElemTy->getPointerTo()->getPointerTo() });
    }
    // ex. liste(nombre) -> { i64, double* }
    return StructType::get(context, { builder.getInt64Ty(), lstElemTy->getPointerTo() });
}