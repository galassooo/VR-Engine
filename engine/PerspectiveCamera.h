#pragma once

/**
 * @class PerspectiveCamera
 * @brief Represents a camera using a perspective projection.
 *
 * The PerspectiveCamera generates a projection matrix that simulates the effect of a camera lens,
 * with objects appearing smaller as their distance from the camera increases. It inherits from
 * the Camera class.
 */
class ENG_API PerspectiveCamera : public Eng::Camera {
public:
   PerspectiveCamera(float fov, float aspect, float nearClip, float farClip);

   glm::mat4 getProjectionMatrix() const override;

   void setAspect(float newAspect); //-> when reshaping the window

   float getNearClip() const;
   float getFarClip() const;

private:
   ///> Field of view angle in degrees
   float fov;
   ///> Aspect ratio (width/height) of the viewport
   float aspect;
   ///> Distance to the near clipping plane
   float nearClip;
   ///> Distance to the far clipping plane
   float farClip;
};
