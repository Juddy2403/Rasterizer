#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}

		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix

			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			viewMatrix = Matrix{
				Vector4 {right, 0},
				Vector4 {up, 0},
				Vector4 {forward, 0},
				Vector4 {origin, 1}
			};
			invViewMatrix = viewMatrix.Inverse();
			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W3

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			//Camera Update Logic
			float deltaTime = pTimer->GetElapsed();
			deltaTime = std::ranges::clamp(deltaTime, 0.005f, 0.01f);

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			const float movementSpeed{ 20.f }, rotationSpeed{ 100.f };

			if (pKeyboardState[SDL_SCANCODE_W]) origin += (movementSpeed * deltaTime) * forward.Normalized();
			if (pKeyboardState[SDL_SCANCODE_S]) origin -= (movementSpeed * deltaTime) * forward.Normalized();
			if (pKeyboardState[SDL_SCANCODE_A]) origin -= (movementSpeed * deltaTime) * right.Normalized();
			if (pKeyboardState[SDL_SCANCODE_D]) origin += (movementSpeed * deltaTime) * right.Normalized();

			if (mouseState == SDL_BUTTON_LEFT && mouseY)	origin -= (movementSpeed * deltaTime) * forward.Normalized() * mouseY;
			if (mouseState == SDL_BUTTON_X2 && mouseY) origin -= (movementSpeed * deltaTime) * up.Normalized() * mouseY;
			if (mouseState == SDL_BUTTON_LEFT && mouseX) totalYaw += rotationSpeed * deltaTime * mouseX;

			if (mouseState == SDL_BUTTON_X1)
			{
				if (!(totalPitch > 88.f && mouseY <= 0) && !(totalPitch < -88.f && mouseY >= 0))
					totalPitch -= rotationSpeed * deltaTime * mouseY;

				totalYaw += rotationSpeed * deltaTime * mouseX;
			}
			Matrix finalRotation{};

			finalRotation = Matrix::CreateRotationX(totalPitch * TO_RADIANS);
			finalRotation *= Matrix::CreateRotationY(totalYaw * TO_RADIANS);

			forward = finalRotation.TransformVector(Vector3::UnitZ);
			forward.Normalize();

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
