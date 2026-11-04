#include "GArcadeCameraSwitcherComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogArcadeCamera, Log, All);

UGArcadeCameraSwitcherComponent::UGArcadeCameraSwitcherComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGArcadeCameraSwitcherComponent::BeginPlay()
{
	Super::BeginPlay();

	for (int32 viewIndex = 0; viewIndex < views.Num(); ++viewIndex)
	{
		if (!ResolveCamera(viewIndex))
		{
			UE_LOG(LogArcadeCamera, Warning, TEXT("%s: view %d (%s) does not point at a camera component of %s."),
				   *GetName(), viewIndex, *views[viewIndex].viewName.ToString(), *GetNameSafe(GetOwner()));
		}
	}
	SetView(initialViewIndex);
}

void UGArcadeCameraSwitcherComponent::CycleView()
{
	if (views.Num() > 0)
	{
		SetView((currentViewIndex + 1) % views.Num());
	}
}

void UGArcadeCameraSwitcherComponent::SetView(int32 viewIndex)
{
	if (!views.IsValidIndex(viewIndex))
	{
		return;
	}

	// Deactivate first: the camera manager takes the first active camera it finds on the view target.
	for (int32 otherIndex = 0; otherIndex < views.Num(); ++otherIndex)
	{
		UCameraComponent* otherCamera = ResolveCamera(otherIndex);
		if (otherCamera && otherIndex != viewIndex)
		{
			otherCamera->Deactivate();
		}
	}
	if (UCameraComponent* camera = ResolveCamera(viewIndex))
	{
		camera->Activate();
	}
	currentViewIndex = viewIndex;
}

FName UGArcadeCameraSwitcherComponent::GetViewName() const
{
	return views.IsValidIndex(currentViewIndex) ? views[currentViewIndex].viewName : NAME_None;
}

UCameraComponent* UGArcadeCameraSwitcherComponent::ResolveCamera(int32 viewIndex) const
{
	return views.IsValidIndex(viewIndex) ? Cast<UCameraComponent>(views[viewIndex].camera.GetComponent(GetOwner())) : nullptr;
}
