#ifndef Camera_hpp
#define Camera_hpp

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <string>

namespace gps {

    enum MOVE_DIRECTION { MOVE_FORWARD, MOVE_BACKWARD, MOVE_RIGHT, MOVE_LEFT, MOVE_UP, MOVE_DOWN};

    class Camera
    {
    public:
        //Camera constructor
        Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp);
        //return the view matrix, using the glm::lookAt() function
        glm::mat4 getViewMatrix();
        //update the camera internal parameters following a camera move event
        void move(MOVE_DIRECTION direction, float speed);
        //update the camera internal parameters following a camera rotate event
        //yaw - camera rotation around the y axis
        //pitch - camera rotation around the x axis
        void rotate(float pitch, float yaw);

        void setCameraPosition(glm::vec3 cameraPosition) {
            this->cameraPosition = cameraPosition;
			updateCameraVectors();
        }

        void setCameraTarget(glm::vec3 cameraTarget) {
            this->cameraTarget = cameraTarget;
            cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
            cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
			updateCameraVectors();
        }

    private:
        glm::vec3 cameraPosition;
        glm::vec3 cameraTarget;
        glm::vec3 cameraFrontDirection;
        glm::vec3 cameraRightDirection;
        glm::vec3 cameraUpDirection;

        glm::vec3 cameraUpInitial;

        void updateCameraVectors();
    };

}

#endif /* Camera_hpp */
