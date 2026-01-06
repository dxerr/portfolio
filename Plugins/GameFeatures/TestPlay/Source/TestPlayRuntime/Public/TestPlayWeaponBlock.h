// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestPlayWeaponBlock.generated.h"

class UBoxComponent;
class USphereComponent;


/**
 * 
 */
UCLASS()
class TESTPLAYRUNTIME_API ATestPlayWeaponBlock : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestPlayWeaponBlock();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

public:	
public:	
	// Name of the component to use for collision blocking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Block", meta = (GetOptions = "GetCollisionComponentNames"))
	FName CollisionComponentName;

	// Runtime activation control
	UFUNCTION(BlueprintCallable, Category = "Weapon Block")
	void SetActive(bool bIsActive);

	// Helper to find attached WeaponBlock from an actor
	UFUNCTION(BlueprintCallable, Category = "Weapon Block", meta = (DefaultToSelf = "OwnerActor"))
	static ATestPlayWeaponBlock* FindWeaponBlock(AActor* OwnerActor);

	UFUNCTION()
	TArray<FString> GetCollisionComponentNames() const;

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> TargetCollisionComponent;

	UPROPERTY(Transient)
	FName OriginalCollisionProfileName;

	UPROPERTY(Transient)
	TEnumAsByte<ECollisionEnabled::Type> OriginalCollisionEnabled;

public:
	// Debug Visualization
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebug;

	virtual void Tick(float DeltaTime) override;
};
