#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "GInteractionFeedback.generated.h"

class STextBlock;
class SWidget;
class UBoxComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class UGInteractionFeedback : public UCameraModifier
{
	GENERATED_BODY()

public:
	UGInteractionFeedback();

	void Show(UPrimitiveComponent* mesh, UBoxComponent* interactionBox, const FText& key, const FText& action);
	void Clear();
	void RemovePrompt();

protected:
	void ModifyPostProcess(float deltaTime, float& blendWeight, FPostProcessSettings& settings) override;

private:
	void SetOutline(UPrimitiveComponent* mesh);
	void CreatePrompt();
	UStaticMeshComponent* GetOutlineBox(UBoxComponent& interactionBox);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> outlineMaterial;
	UPROPERTY()
	TObjectPtr<UStaticMesh> outlineBoxMesh;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> outlineBox;

	TWeakObjectPtr<UPrimitiveComponent> outlinedMesh;
	TWeakObjectPtr<UBoxComponent> outlinedBoxSource;
	int32 previousStencil = 0;
	bool bPreviouslyRenderedCustomDepth = false;

	TSharedPtr<SWidget> prompt;
	TSharedPtr<STextBlock> keyText;
	TSharedPtr<STextBlock> actionText;
};
