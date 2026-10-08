#pragma once

#include "CoreMinimal.h"
#include "Characters/CharacterBase.h"
#include "GameFramework/Actor.h"
#include "Enemy.generated.h"

class UAudioManager;
enum class EActionId : uint8;
class UActionDataAsset;
class UEnemyArchetypeDataAsset;


UCLASS()
class EGNISDEF_API AEnemy : public ACharacterBase
{
	GENERATED_BODY()

public:

	AEnemy();
	virtual void Tick(float DeltaTime) override;
	//Selecionar Arquetipo
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Enemy|Data")
	TObjectPtr<UEnemyArchetypeDataAsset> ArchetypeData = nullptr;

	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category ="Enemy|AI")
	bool bDebugMovementOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Combat")
	float HealAmount = 8.f;

	// Mismo GE_CardDamage reutilizado ya en las cartas — Add + Set By Caller sirve igual
	// para el ataque (negativo) que para la curación (positivo) del enemigo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat")
	TSubclassOf<class UGameplayEffect> AttackEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat")
	TSubclassOf<class UGameplayEffect> HealEffect;
	
	UFUNCTION(BlueprintCallable, Category ="Enemy|AI")
	void MoveTowardClosesPlayer();

	UFUNCTION(BlueprintCallable, Category ="Enemy|AI")
	void MakeAction();

	UFUNCTION(BlueprintCallable,Category = "Enemy|Actions")
	const UActionDataAsset* GetActionById(EActionId ActionId) const;

protected:

	virtual void BeginPlay() override;

private:

	ACharacterBase* FindClosestPlayer() const;
	static int32 ManhattanDistance(const FTileCoord& A, const FTileCoord& B);

	ACharacterBase* FindBestTarget(int32 _Range, int32 _BestRatio, int32 _Team);
};
