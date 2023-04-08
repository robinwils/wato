#include "input.h"
#include <string.h>

void MouseState::clear()
{
	pos.x = -1.0f;
	pos.y = -1.0f;
	scroll.x = 0.0f;
	scroll.y = 0.0f;
	memset(buttons, 0, sizeof(buttons));
}

Mouse::Button toMouseButton(int32_t _button)
{
	switch (_button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		return Mouse::Button::Left;
	case GLFW_MOUSE_BUTTON_RIGHT:
		return Mouse::Button::Right;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		return Mouse::Button::Middle;
	default:
		return Mouse::Button::Unknown;
	}
}

Button::Action toMouseAction(int32_t _action)
{
	switch (_action)
	{
	case GLFW_PRESS:
		return Button::Action::Press;
	case GLFW_RELEASE:
		return Button::Action::Release;
	case GLFW_REPEAT:
		return Button::Action::Repeat;
	default:
		return Button::Action::Unknown;
	}
}

static void cursorPosCb(GLFWwindow* _window, double _xpos, double _ypos)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(_window));

	input->setMousePos(_xpos, _ypos);
}


static void mouseButtonCb(GLFWwindow* _window, int32_t _button, int32_t _action, int32_t _mods)
{
	Input* input = static_cast<Input*>(glfwGetWindowUserPointer(_window));
	Mouse::Button button = toMouseButton(_button);

	input->setMouseButtonPressed(button, toMouseAction(_action));

	if (_mods & GLFW_MOD_ALT)
	{
		input->setMouseButtonModifier(button, ModifierKey::Alt);
	}
}

static void scrollCb(GLFWwindow* _window, double _xoffset, double _yoffset)
{
	static_cast<Input*>(glfwGetWindowUserPointer(_window))->setMouseScroll(_xoffset, _yoffset);
}


Input::Input(GLFWwindow *_window)
	: mouseState(), m_window(_window)
{
}

void Input::init()
{
	//glfwSetKeyCallback(m_window[0], keyCb);
	//glfwSetCharCallback(m_window[0], charCb);
	glfwSetScrollCallback(m_window, scrollCb);
	glfwSetCursorPosCallback(m_window, cursorPosCb);
	glfwSetMouseButtonCallback(m_window, mouseButtonCb);
	//glfwSetWindowSizeCallback(m_window[0], windowSizeCb);
	//glfwSetDropCallback(m_window[0], dropFileCb);

}

void Input::setMouseButtonPressed(Mouse::Button _button, Button::Action _action)
{
	mouseState.buttons[_button].action = _action;
}

void Input::setMouseButtonModifier(Mouse::Button _button, ModifierKey _mod)
{
	mouseState.buttons[_button].modifiers[_mod] = true;
}

void Input::setMousePos(double _x, double _y)
{
	mouseState.pos.x = _x;
	mouseState.pos.y = _y;
}

void Input::setMouseScroll(double _xoffset, double _yoffset)
{
	mouseState.scroll.x = _xoffset;
	mouseState.scroll.y = _yoffset;
}

bool Input::isMouseButtonPressed(Mouse::Button _button)
{
	return mouseState.buttons[_button].action == Button::Press;
}

void Input::clear()
{
	mouseState.clear();
}
