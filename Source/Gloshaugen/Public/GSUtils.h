#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

namespace GSUtils
{
	inline UMaterialInterface* BaseMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		}
		return Cached.Get();
	}

	/** Gives a primitive a flat color using the engine's basic shape material. */
	inline void SetColor(UPrimitiveComponent* Comp, const FLinearColor& Color, int32 Slot = 0)
	{
		if (!Comp) return;
		if (UMaterialInterface* Base = BaseMaterial())
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Comp);
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Comp->SetMaterial(Slot, MID);
		}
	}

	inline UStaticMesh* Cube()     { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")); }
	inline UStaticMesh* Cylinder() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
	inline UStaticMesh* Sphere()   { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")); }
	inline UStaticMesh* Cone()     { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")); }
}
