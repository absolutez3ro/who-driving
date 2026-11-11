#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GCombatEditorLibrary.generated.h"

class UAnimBlueprint;
class USkeletalMesh;
class USkeleton;
class UAnimSequence;
class APlayerController;

UCLASS()
class WHOSDRIVINGCOMBATEDITOR_API UGCombatEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Vehicle|Editor|Testing")
	static bool InjectPlayerKey(APlayerController* controller, FName keyName, bool bPressed);

	UFUNCTION(BlueprintCallable, Category = "Combat|Editor")
	static bool CopyM8Packages(const TArray<FString>& sourcePackages);
	UFUNCTION(BlueprintCallable, Category = "Combat|Editor")
	static UAnimBlueprint* CreateCombatAnimBlueprint(const FString& packageName, USkeleton* skeleton,
													 UAnimSequence* basePose);
	UFUNCTION(BlueprintCallable, Category = "Combat|Editor")
	static bool AddMeshSocket(USkeletalMesh* mesh, FName socketName, FName boneName, FTransform offset);
	// Skeleton sockets are shared by all of its meshes and do not require resaving the heavy mesh assets.
	UFUNCTION(BlueprintCallable, Category = "Combat|Editor")
	static bool AddSkeletonSocket(USkeleton* skeleton, FName socketName, FName boneName, FTransform offset);
	UFUNCTION(BlueprintCallable, Category = "Combat|Editor")
	static bool AlignCombatHand(UAnimBlueprint* blueprint);
	UFUNCTION(BlueprintCallable, Category = "Combat|Editor")
	static bool AlignCombatLeftHand(UAnimBlueprint* blueprint);
	UFUNCTION(BlueprintCallable, Category = "Combat|Editor")
	static bool SetCombatArmStretch(UAnimBlueprint* blueprint, double maximumScale);
};
