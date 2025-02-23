#pragma once
/**
 * @class OrthographicCamera
 * @brief Represents a camera using an orthographic projection.
 *
 * The OrthographicCamera generates a projection matrix based on user-defined
 * orthographic bounds and clipping planes. It inherits from the Camera class
 */
class ENG_API OrthographicCamera : public Eng::Camera {
public:
	OrthographicCamera(float left, float right, float bottom, float top, float nearClip, float farClip);
	glm::mat4 getProjectionMatrix() const override;

private:
	///< Left clipping plane
	float left;
	///< Right clipping plane
	float right;
	///< Bottom clipping plane
	float bottom;
	///< Top clipping plane
	float top;
	///< Near clipping plane
	float nearClip;
	///< Far clipping plane
	float farClip;
};