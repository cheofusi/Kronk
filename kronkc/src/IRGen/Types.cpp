#include "Nodes.h"
#include "irGenAide.h"

Type* PrimitiveTyId::typegen() {
    if (prityId == "entier") {
        return builder.getInt64Ty();
    }
    else if (prityId == "reel") {
        return builder.getDoubleTy();
    }

    // This will never happen as the parser cannot miss this.
    return irGenAide::LogCodeGenError("Primitive Type not recognized");
}


Type* EntityTyId::typegen() {
    return EntityTypes[enttyId];
}


Type* ListTyId::typegen() {
    auto lstElemTy = lsttyId->typegen();
    // The first element of a liste entity is the its size. The second is a ptr to its element type

    if(lstElemTy->isStructTy()) {
        // this is because lists of entities hold pointers to those entities, the entities themselves
        // ex. liste(liste(entier)) -> { i64, { i64, i64* }** }, liste(Point) -> { i64, Point** }
        return StructType::get(context, { builder.getInt64Ty(), lstElemTy->getPointerTo()->getPointerTo() });
    }
    // ex. liste(entier) -> { i64, i64* }
    return StructType::get(context, { builder.getInt64Ty(), lstElemTy->getPointerTo() });
}