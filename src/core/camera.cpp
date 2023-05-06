#include "camera.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

void Camera::drawImgui()
{
    ImGui::Text("Camera Setting");
    ImGui::DragFloat3("Position", glm::value_ptr(m_position), -5.0f, 5.0f);
    ImGui::DragFloat3("Direction", glm::value_ptr(m_dir), -2.0f, 2.0f);
    ImGui::DragFloat("FoV (Degree)", &m_fov, 10.0f, 120.0f);
    ImGui::DragFloat("Speed", &m_speed, 0.1f, 0.01f, 5.0f, "%.03f");
}

void Camera::update(const Input& _input, double  _timeDelta)
{
    float speed = m_speed * _timeDelta;
    if (_input.isKeyPressed(Keyboard::Key::W) || _input.isKeyRepeat(Keyboard::Key::W))
    {
        m_position += speed * m_front;
    }
    if (_input.isKeyPressed(Keyboard::Key::A) || _input.isKeyRepeat(Keyboard::Key::A))
    {
        m_position += speed * right();
    }
    if (_input.isKeyPressed(Keyboard::Key::S) || _input.isKeyRepeat(Keyboard::Key::S))
    {
        m_position -= speed * m_front;
    }
    if (_input.isKeyPressed(Keyboard::Key::D) || _input.isKeyRepeat(Keyboard::Key::D))
    {
        m_position -= speed * right();
    }
}
