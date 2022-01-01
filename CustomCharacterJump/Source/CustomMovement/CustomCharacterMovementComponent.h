// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class CUSTOMMOVEMENT_API UCustomCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float delta_time, ELevelTick tick_type, FActorComponentTickFunction* tick_function) override;
	virtual bool DoJump(bool bReplayingMoves) override;
	virtual bool IsFalling() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Movement", meta = (DisplayName = "Jump Curve"))
	UCurveFloat* m_jump_curve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Movement", meta = (DisplayName = "Fall Curve"))
	UCurveFloat* m_fall_curve;

private:
	void SetCustomFallingMode();
	void ProcessCustomJump(float delta_time);
	void ProcessCustomFalling(float delta_time);

	bool CustomFindFloor(FFindFloorResult& out_floor_result, const FVector start, const FVector end);

	bool GetJumpApexTime(float& apex_time);

	bool m_is_jumping;
	float m_jump_time;
	float m_prev_jump_curve_value;
	
	float m_jump_min_time;
	float m_jump_max_time;


	bool m_is_falling;
	float m_fall_time;
	float m_prev_fall_curve_value;

	bool m_is_valid_jump_curve = false;
	float m_jump_apex_time;
	float m_jump_apex_height;
	float m_prev_jump_time;

	bool m_print_height = true;
	float m_start_height;
};
