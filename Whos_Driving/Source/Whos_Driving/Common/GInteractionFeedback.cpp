#include "GInteractionFeedback.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// Matches the stencil selected by M_InteractionOutline.
	constexpr int32 interactionStencil = 250;
}

UGInteractionFeedback::UGInteractionFeedback()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> material(
		TEXT("/Game/Materials/UIMaterials/M_InteractionOutline"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> cube(TEXT("/Engine/BasicShapes/Cube"));
	outlineMaterial = material.Object;
	outlineBoxMesh = cube.Object;
}

void UGInteractionFeedback::Show(UPrimitiveComponent* mesh, UBoxComponent* interactionBox, const FText& key, const FText& action)
{
	if (!mesh && interactionBox)
	{
		mesh = GetOutlineBox(*interactionBox);
	}
	SetOutline(mesh);
	if (!prompt)
	{
		CreatePrompt();
	}
	if (prompt)
	{
		if (!keyText->GetText().EqualTo(key))
		{
			keyText->SetText(key);
		}
		if (!actionText->GetText().EqualTo(action))
		{
			actionText->SetText(action);
		}
		prompt->SetVisibility(EVisibility::HitTestInvisible);
	}
}

void UGInteractionFeedback::Clear()
{
	SetOutline(nullptr);
	if (prompt)
	{
		prompt->SetVisibility(EVisibility::Collapsed);
	}
}

void UGInteractionFeedback::RemovePrompt()
{
	Clear();
	if (IsValid(outlineBox))
	{
		outlineBox->DestroyComponent();
	}
	if (prompt && CameraOwner && CameraOwner->PCOwner)
	{
		if (ULocalPlayer* player = CameraOwner->PCOwner->GetLocalPlayer(); player && player->ViewportClient)
		{
			player->ViewportClient->RemoveViewportWidgetForPlayer(player, prompt.ToSharedRef());
		}
	}
	prompt.Reset();
	keyText.Reset();
	actionText.Reset();
}

void UGInteractionFeedback::ModifyPostProcess(float deltaTime, float& blendWeight, FPostProcessSettings& settings)
{
	if (outlineMaterial && outlinedMesh.IsValid())
	{
		blendWeight = 1.0f;
		settings.AddBlendable(outlineMaterial, 1.0f);
	}
}

void UGInteractionFeedback::SetOutline(UPrimitiveComponent* mesh)
{
	if (outlinedMesh.Get() == mesh)
	{
		return;
	}
	if (UPrimitiveComponent* previous = outlinedMesh.Get())
	{
		previous->SetRenderCustomDepth(bPreviouslyRenderedCustomDepth);
		previous->SetCustomDepthStencilValue(previousStencil);
	}
	outlinedMesh = mesh;
	if (mesh && outlineMaterial)
	{
		bPreviouslyRenderedCustomDepth = mesh->bRenderCustomDepth;
		previousStencil = mesh->CustomDepthStencilValue;
		mesh->SetCustomDepthStencilValue(interactionStencil);
		mesh->SetRenderCustomDepth(true);
	}
}

UStaticMeshComponent* UGInteractionFeedback::GetOutlineBox(UBoxComponent& interactionBox)
{
	if (!outlineMaterial || !outlineBoxMesh)
	{
		return nullptr;
	}
	if (IsValid(outlineBox) && outlineBox->GetOwner() != interactionBox.GetOwner())
	{
		SetOutline(nullptr);
		outlineBox->DestroyComponent();
		outlineBox = nullptr;
	}
	if (!IsValid(outlineBox))
	{
		// Some interactions, including the radio, only have a collision box.
		outlineBox = NewObject<UStaticMeshComponent>(interactionBox.GetOwner());
		outlineBox->SetStaticMesh(outlineBoxMesh);
		outlineBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		outlineBox->SetCastShadow(false);
		outlineBox->SetRenderInMainPass(false);
		outlineBox->SetRenderInDepthPass(false);
		outlineBox->RegisterComponent();
		outlinedBoxSource.Reset();
	}
	if (outlinedBoxSource.Get() != &interactionBox)
	{
		outlineBox->AttachToComponent(&interactionBox, FAttachmentTransformRules::SnapToTargetIncludingScale);
		outlineBox->SetRelativeScale3D(interactionBox.GetUnscaledBoxExtent() / 50.0f);
		outlinedBoxSource = &interactionBox;
	}
	return outlineBox;
}

void UGInteractionFeedback::CreatePrompt()
{
	ULocalPlayer* player = CameraOwner && CameraOwner->PCOwner ? CameraOwner->PCOwner->GetLocalPlayer() : nullptr;
	if (!player || !player->ViewportClient)
	{
		return;
	}
	const FSlateBrush* whiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	prompt = SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(0.0f, 78.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(whiteBrush)
			.BorderBackgroundColor(FLinearColor(0.018f, 0.025f, 0.032f, 0.94f))
			.Padding(FMargin(6.0f, 5.0f, 12.0f, 5.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SBorder)
					.BorderImage(whiteBrush)
					.BorderBackgroundColor(FLinearColor(0.68f, 0.92f, 0.96f))
					.Padding(FMargin(7.0f, 2.0f))
					[
						SAssignNew(keyText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 13))
						.ColorAndOpacity(FLinearColor(0.018f, 0.025f, 0.032f))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SAssignNew(actionText, STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Medium"), 14))
					.ColorAndOpacity(FLinearColor::White)
				]
			]
		];
	player->ViewportClient->AddViewportWidgetForPlayer(player, prompt.ToSharedRef(), 5);
}
