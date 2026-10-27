// Fill out your copyright notice in the Description page of Project Settings.


#include "GBaseEnemyCar.h"
#include "GEnemyBaseDataAsset.h"

// Sets default values
AGBaseEnemyCar::AGBaseEnemyCar()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.

}

// Called when the game starts or when spawned
void AGBaseEnemyCar::BeginPlay()
{
	Super::BeginPlay();
	maxHp = baseData->maxHp;
	currentHp = maxHp;
	scrapPrice = baseData->scrapPrice;
	USkeletalMesh* TargetMesh = baseData->mesh;
	if (TargetMesh)
	{
		// The car body is the root; the seated crew are skeletal meshes too, so don't just take the first one found
		USkeletalMeshComponent* MeshComp = Cast<USkeletalMeshComponent>(GetRootComponent());
		if (!MeshComp)
		{
			MeshComp = FindComponentByClass<USkeletalMeshComponent>();
		}

		if (!MeshComp)
		{
			UE_LOG(LogTemp, Error, TEXT("No skeletal mesh found for enemy"));
			MeshComp = NewObject<USkeletalMeshComponent>(this, USkeletalMeshComponent::StaticClass());
			MeshComp->RegisterComponent(); 
			MeshComp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
		MeshComp->SetSkeletalMesh(TargetMesh);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Skeletal Mesh from Data Asset on %s"), *GetName());
	}
}

// Called every frame
void AGBaseEnemyCar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGBaseEnemyCar::MulticastShotFired_Implementation(FVector target, bool bHit)
{
	OnShotFired(target, bHit);
}

void AGBaseEnemyCar::HandleDeath() {
	if (!HasAuthority())
	{
		return;
	}
	Destroy(true, false);
}

// Called to bind functionality to input
float AGBaseEnemyCar::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority())
	{
		return 0.0f;
	}
	if (DamageAmount>0)
	{
		currentHp -= DamageAmount;
		UE_LOG(LogTemp, Log, TEXT("damage: %f"), DamageAmount);

		UE_LOG(LogTemp, Log, TEXT("hp left: %f / %f"), currentHp, maxHp);
		if (currentHp <= 0) 
		{
			OnDeath.Broadcast(scrapPrice);
			HandleDeath();
		}
	}
	return DamageAmount;
}

