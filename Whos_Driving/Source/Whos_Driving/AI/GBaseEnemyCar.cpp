// Fill out your copyright notice in the Description page of Project Settings.


#include "GBaseEnemyCar.h"
#include "GEnemyBaseDataAsset.h"
#include "Kismet/GameplayStatics.h"

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
	USkeletalMesh* targetMesh = baseData->mesh;
	if (targetMesh)
	{
		// The car body is the root; the seated crew are skeletal meshes too, so don't just take the first one found
		USkeletalMeshComponent* meshComponent = Cast<USkeletalMeshComponent>(GetRootComponent());
		if (!meshComponent)
		{
			meshComponent = FindComponentByClass<USkeletalMeshComponent>();
		}

		if (!meshComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("No skeletal mesh found for enemy"));
			meshComponent = NewObject<USkeletalMeshComponent>(this, USkeletalMeshComponent::StaticClass());
			meshComponent->RegisterComponent();
			meshComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
		meshComponent->SetSkeletalMesh(targetMesh);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Skeletal Mesh from Data Asset on %s"), *GetName());
	}
}

// Called every frame
void AGBaseEnemyCar::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

}

void AGBaseEnemyCar::PlayShotSound_Implementation()
{
	if (hitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, hitSound, GetActorLocation());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("NO SOUND"));
	}
}

void AGBaseEnemyCar::PlayMissSound_Implementation()
{
	if (missSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, missSound, GetActorLocation());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("NO SOUND"));
	}
}

void AGBaseEnemyCar::HandleDeath()
{
	if (!HasAuthority())
	{
		return;
	}
	Destroy(true, false);
}

// Applies incoming damage on the server
float AGBaseEnemyCar::TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser)
{
	if (!HasAuthority())
	{
		return 0.0f;
	}
	if (damageAmount > 0)
	{
		currentHp -= damageAmount;
		UE_LOG(LogTemp, Log, TEXT("damage: %f"), damageAmount);

		UE_LOG(LogTemp, Log, TEXT("hp left: %f / %f"), currentHp, maxHp);
		if (currentHp <= 0)
		{
			onDeath.Broadcast(scrapPrice);
			HandleDeath();
		}
	}
	return damageAmount;
}
