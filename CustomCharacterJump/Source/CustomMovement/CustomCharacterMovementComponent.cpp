// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomCharacterMovementComponent.h"

#include "GameFramework/Character.h"
#include "Components/capsuleComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

void UCustomCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	m_is_falling = false;

	if (m_jump_curve)
	{
		m_jump_curve->GetTimeRange(m_jump_min_time, m_jump_max_time);
		float min_height;
		m_jump_curve->GetValueRange(min_height, m_jump_apex_height);

		m_is_valid_jump_curve = GetJumpApexTime(m_jump_apex_time);
		if (!m_is_valid_jump_curve)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to find apex time for given jump curve."));
		}
	}
}

void UCustomCharacterMovementComponent::TickComponent(float delta_time, ELevelTick tick_type, FActorComponentTickFunction* tick_function)
{
	Super::TickComponent(delta_time, tick_type, tick_function);

	ProcessCustomJump(delta_time);
	ProcessCustomFalling(delta_time);
}

bool UCustomCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
	if (CharacterOwner && CharacterOwner->CanJump())
	{
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.0f)
		{
			if (m_is_valid_jump_curve && m_jump_curve)
			{
				if (!m_is_jumping)
				{
					SetMovementMode(EMovementMode::MOVE_Flying);

					m_is_jumping = true;
					m_is_falling = false;
					m_jump_time = m_jump_min_time;
					m_prev_jump_time = m_jump_time;
					m_prev_jump_curve_value = m_jump_curve->GetFloatValue(m_jump_min_time);
					m_start_height = UpdatedComponent->GetComponentLocation().Z;
					m_print_height = true;

					return true;
				}
				
				return false;
			}
			else
			{
				m_is_falling = false;
				return Super::DoJump(bReplayingMoves);
			}
		}
	}

	return false;
}

bool UCustomCharacterMovementComponent::IsFalling() const
{
	if (m_is_valid_jump_curve && m_jump_curve)
	{
		return Super::IsFalling() || m_is_jumping || m_is_falling;
	}

	return Super::IsFalling();
}

void UCustomCharacterMovementComponent::SetCustomFallingMode()
{
	if (m_fall_curve)
	{
		m_is_falling = true;
		m_fall_time = 0.0f;
		m_prev_fall_curve_value = 0.0f;
		Velocity.Z = 0.0f;

		SetMovementMode(EMovementMode::MOVE_Flying);
	}
	else
	{
		SetMovementMode(EMovementMode::MOVE_Falling);
	}
}

void UCustomCharacterMovementComponent::ProcessCustomJump(float delta_time)
{
	if (m_is_valid_jump_curve && m_is_jumping && m_jump_curve)
	{
		m_jump_time += delta_time;

		if (m_jump_time <= m_jump_max_time)
		{
			float jump_curve_value = m_jump_curve->GetFloatValue(m_jump_time);

			// Make sure that character reach the jump apex height and notify character about reaching apex
			bool is_jump_apex_reached = m_prev_jump_time < m_jump_apex_time && m_jump_time > m_jump_apex_time;
			if (is_jump_apex_reached)
			{
				m_jump_time = m_jump_apex_time;
				jump_curve_value = m_jump_apex_height;
			}
			m_prev_jump_time = m_jump_time;
			
			float jump_curve_value_delta = jump_curve_value - m_prev_jump_curve_value;
			m_prev_jump_curve_value = jump_curve_value;

			Velocity.Z = 0.0f;
			float y_velocity = jump_curve_value_delta / delta_time;

			const FVector capsule_location = UpdatedComponent->GetComponentLocation();
			FVector destination_location = capsule_location + FVector(0.0f, 0.0f, jump_curve_value_delta);

			// Check for roof collisions if character is moving up
			if (y_velocity > 0.0f)
			{
				FCollisionQueryParams roof_check_collision_params;
				roof_check_collision_params.AddIgnoredActor(CharacterOwner);
				FCollisionShape capsule_shape = FCollisionShape::MakeCapsule(CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

				FHitResult roof_hit_result;
				bool is_blocking_hit = GetWorld()->SweepSingleByProfile(roof_hit_result, capsule_location, destination_location, CharacterOwner->GetActorRotation().Quaternion(), CharacterOwner->GetCapsuleComponent()->GetCollisionProfileName(), capsule_shape, roof_check_collision_params);
				if (is_blocking_hit)
				{
					// Roof collision hit
					SetCustomFallingMode();

					m_is_jumping = false;
					CharacterOwner->ResetJumpState();

					// Reset vertical velocity and let gravity do the work
					Velocity.Z = 0.0f;

					// Take character to safe location where its not hitting roof.
					// This can be improved by using the impact location and using that
					destination_location = capsule_location;
				}
			}

			if (y_velocity < 0.0f)
			{
				if (m_print_height)
				{
					m_print_height = false;
					float current_height = UpdatedComponent->GetComponentLocation().Z;
					UE_LOG(LogTemp, Warning, TEXT("Jump Apex Height: %f"), (current_height - m_start_height));
				}
				// Falling, check for floor
				FFindFloorResult floor_result;
				bool is_floor = CustomFindFloor(floor_result, capsule_location, destination_location);
				const float floor_distance = floor_result.GetDistanceToFloor();
				if (is_floor)
				{
					if (floor_distance < FMath::Abs(jump_curve_value_delta))
					{
						destination_location = capsule_location - FVector(0.0f, 0.0f, jump_curve_value_delta);
					}

					SetMovementMode(EMovementMode::MOVE_Walking);

					m_is_jumping = false;
					CharacterOwner->ResetJumpState();
				}
			}

			FLatentActionInfo latent_info;
			latent_info.CallbackTarget = this;
			UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destination_location, CharacterOwner->GetActorRotation(), false, false, 0.0f, true, EMoveComponentAction::Type::Move, latent_info);

			if (is_jump_apex_reached)
			{
				if (bNotifyApex)
				{
					NotifyJumpApex();
				}
			}
		}
		else
		{
			// reached the end of jump curve
			const FVector capsule_location = UpdatedComponent->GetComponentLocation();
			FFindFloorResult floor_result;
			FindFloor(capsule_location, floor_result, false);
			if (floor_result.IsWalkableFloor() && IsValidLandingSpot(capsule_location, floor_result.HitResult))
			{
				SetMovementMode(EMovementMode::MOVE_Walking);
			}
			else
			{
				SetCustomFallingMode();
			}

			m_is_jumping = false;
			CharacterOwner->ResetJumpState();
		}
	}
}

void UCustomCharacterMovementComponent::ProcessCustomFalling(float delta_time)
{
	if (m_fall_curve)
	{
		if (m_is_falling)
		{
			m_fall_time += delta_time;

			float fall_curve_value = m_fall_curve->GetFloatValue(m_fall_time);
			float delta_fall_curve_value = fall_curve_value - m_prev_fall_curve_value;
			m_prev_fall_curve_value = fall_curve_value;

			// Setting velocity;
			//Velocity.Z = delta_fall_curve_value / delta_time;
			Velocity.Z = 0.0f;

			FVector capsule_location = UpdatedComponent->GetComponentLocation();
			FVector destination_location = capsule_location + FVector(0.0f, 0.0f, delta_fall_curve_value);

			FFindFloorResult floor_result;
			const bool is_floor = CustomFindFloor(floor_result, capsule_location, destination_location);
			if (is_floor)
			{
				const float floor_distance = floor_result.GetDistanceToFloor();

				if (floor_distance < FMath::Abs(delta_fall_curve_value))
				{
					// Since going down, substrace the floor distance
					destination_location = capsule_location - FVector(0.0f, 0.0f, floor_distance);

					m_is_falling = false;

					// Stopping the character and cancelling all the momentum carried from before the jump/fall
					// Remove if you want to carry the momentum
					Velocity = FVector::ZeroVector;

					SetMovementMode(EMovementMode::MOVE_Walking);
				}
			}

			FLatentActionInfo latent_info;
			latent_info.CallbackTarget = this;
			UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destination_location, CharacterOwner->GetActorRotation(), false, false, 0.0f, true, EMoveComponentAction::Move, latent_info);
		}
		else if (MovementMode == EMovementMode::MOVE_Falling)
		{
			// Dropping down from edge while walking
			// Note that this will effect default jump since default jump uses falling mode
			SetCustomFallingMode();
		}
	}
}


bool UCustomCharacterMovementComponent::CustomFindFloor(FFindFloorResult& out_floor_result, const FVector start, const FVector end)
{
	FCollisionQueryParams floor_check_collision_params;
	floor_check_collision_params.AddIgnoredActor(CharacterOwner);
	FCollisionShape capsule_shape = FCollisionShape::MakeCapsule(CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	FHitResult floor_hit_result;
	bool is_blocking_hit = GetWorld()->SweepSingleByProfile(floor_hit_result, start, end, CharacterOwner->GetActorRotation().Quaternion(), CharacterOwner->GetCapsuleComponent()->GetCollisionProfileName(), capsule_shape, floor_check_collision_params);

	if (is_blocking_hit)
	{
		FindFloor(start, out_floor_result, false, &floor_hit_result);

		return (out_floor_result.IsWalkableFloor() && IsValidLandingSpot(start, out_floor_result.HitResult));
	}

	return false;
}

//#pragma optimize("", off)
bool UCustomCharacterMovementComponent::GetJumpApexTime(float& apex_time)
{
	const FVector time_axis = FVector(1.0f, 0.0f, 0.0f);
	
	// Getting jump apex height
	float jump_min, jump_apex_height;
	m_jump_curve->GetValueRange(jump_min, jump_apex_height);

	int iterations = 5;
	float step_size = 0.05f;

	// Getting curve time range
	float start, end;
	m_jump_curve->GetTimeRange(start, end);

	float p1_x, p2_x, p3_x;

	FVector p1, p2, p3;
	bool is_apex_found = false;

	while (iterations != 0)
	{
		is_apex_found = false;

		p1_x = start;
		p2_x = p1_x + step_size;
		p3_x = p2_x + step_size;

		if (p2_x > end)
		{
			// Reached end of curve
			return false;
		}

		if (p3_x > end)
		{
			// Clamping p3 to curve range
			p3_x = end;
		}

		while (!is_apex_found)
		{
			float p1_y = m_jump_curve->GetFloatValue(p1_x);
			if (p1_y == jump_apex_height)
			{
				// Early out
				apex_time = p1_x;
				return true;
			}

			float p2_y = m_jump_curve->GetFloatValue(p2_x);
			if (p2_y == jump_apex_height)
			{
				// Early out
				apex_time = p2_x;
				return true;
			}

			float p3_y = m_jump_curve->GetFloatValue(p3_x);
			if (p3_y == jump_apex_height)
			{
				// Early out
				apex_time = p3_x;
				return true;
			}

			p1 = FVector(p1_x, p1_y, 0.0f);
			p2 = FVector(p2_x, p2_y, 0.0f);
			p3 = FVector(p3_x, p3_y, 0.0f);

			FVector normal_1 = FVector::CrossProduct(time_axis, p2 - p1);
			FVector normal_2 = FVector::CrossProduct(time_axis, p3 - p2);

			if (normal_1.Z == 0.0f || normal_2.Z == 0.0f)
			{
				// Curve is parallel to time axis. Considering the flat curve as apex.
				apex_time = p2_x;
				return true;
			}

			// If p2-p1 is going up the slope and p3-p2 is going down the slope, then apex has to be in between p1 and p3
			is_apex_found = normal_1.Z > 0.0f && normal_2.Z < 0.0f;

			if (!is_apex_found)
			{
				p1_x += step_size;
				p2_x += step_size;
				p3_x += step_size;
			}
		}

		--iterations;
		start = p1_x;
		end = p3_x;
		step_size = step_size / 2.0f;
	}

	// Get the mid point of p1 and p3 as the apex time
	apex_time = (p3_x + p1_x) / 2.0f;

	return is_apex_found;
}