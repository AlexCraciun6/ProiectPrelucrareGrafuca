#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;
        this->cameraUpInitial = cameraUp;

        //TODO - Update the rest of camera parameters
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
        updateCameraVectors();

    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        //TODO
        if (direction == MOVE_FORWARD)
            cameraPosition += speed * cameraFrontDirection;
        if (direction == MOVE_BACKWARD)
            cameraPosition -= speed * cameraFrontDirection;
        if (direction == MOVE_RIGHT)
            cameraPosition += speed * cameraRightDirection;
        if (direction == MOVE_LEFT)
            cameraPosition -= speed * cameraRightDirection;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        //TODO
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(front);
        updateCameraVectors();
    }

    // Helper function to update the camera's right and up vectors
    void Camera::updateCameraVectors() {
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpInitial));
        /// salvezi glm::vec3 cameraUp din constructor ca atribut al clasei 
        // caluclaezi cameraRightDirection cu el in loc de cameraUpDirection
        ///     cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUp));
        // nu o sa mai fie dependenta intre cele 2, cameraRightDirection si cameraUpDirection
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }
}