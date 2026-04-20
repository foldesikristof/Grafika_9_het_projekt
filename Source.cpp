#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

#define NOMINMAX
#include <windows.h>

using namespace std;
using namespace glm;

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 600;
const float CONTROL_POINT_DIAMETER = 9.0f;
const float CONTROL_POINT_RADIUS = CONTROL_POINT_DIAMETER / 2.0f;

GLFWwindow* window = nullptr;

vector<vec2> controlPoints = {
    vec2(100.0f, 100.0f),
    vec2(180.0f, 450.0f),
    vec2(420.0f, 450.0f),
    vec2(500.0f, 100.0f)
};

int selectedPoint = -1;
bool dragging = false;

vec2 screenToWorld(double x, double y)
{
    return vec2((float)x, (float)(WINDOW_HEIGHT - y));
}

float distance2D(const vec2& a, const vec2& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}

int findControlPoint(const vec2& mousePos)
{
    for (int i = (int)controlPoints.size() - 1; i >= 0; --i)
    {
        if (distance2D(mousePos, controlPoints[i]) <= CONTROL_POINT_RADIUS + 2.0f)
            return i;
    }
    return -1;
}

double binomialCoeff(int n, int k)
{
    if (k < 0 || k > n) return 0.0;
    if (k == 0 || k == n) return 1.0;

    if (k > n - k)
        k = n - k;

    double result = 1.0;
    for (int i = 1; i <= k; ++i)
    {
        result *= (double)(n - k + i);
        result /= (double)i;
    }
    return result;
}

vec2 bezierPoint(const vector<vec2>& points, double t)
{
    int n = (int)points.size() - 1;
    dvec2 result(0.0, 0.0);

    for (int i = 0; i <= n; ++i)
    {
        double B = binomialCoeff(n, i) *
            pow(1.0 - t, n - i) *
            pow(t, i);

        result += B * dvec2(points[i].x, points[i].y);
    }

    return vec2((float)result.x, (float)result.y);
}

int estimateCurveSegments()
{
    if (controlPoints.size() < 2)
        return 0;

    double polyLength = 0.0;
    for (size_t i = 1; i < controlPoints.size(); ++i)
        polyLength += length(controlPoints[i] - controlPoints[i - 1]);

    int segments = std::max(300, (int)(polyLength * 1.5));
    segments = std::max(segments, (int)controlPoints.size() * 100);
    segments = std::min(segments, 3000);

    return segments;
}

void drawControlPolygon()
{
    if (controlPoints.size() < 2)
        return;

    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);;   // háttér szín
    glLineWidth(2.0f);

    glBegin(GL_LINE_STRIP);
    for (const auto& p : controlPoints)
        glVertex2f(p.x, p.y);
    glEnd();
}

void drawBezierCurve()
{
    if (controlPoints.size() < 2)
        return;

    int segments = estimateCurveSegments();

    glColor3f(1.0f, 0.0f, 0.0f);      // piros görbe
    glLineWidth(3.0f);

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        double t = (double)i / (double)segments;
        vec2 p = bezierPoint(controlPoints, t);
        glVertex2f(p.x, p.y);
    }
    glEnd();
}

void drawControlPoints()
{
    glEnable(GL_POINT_SMOOTH);
    glPointSize(CONTROL_POINT_DIAMETER);
    glColor3f(0.0f, 0.0f, 1.0f);      // kék kontrollpontok

    glBegin(GL_POINTS);
    for (const auto& p : controlPoints)
        glVertex2f(p.x, p.y);
    glEnd();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    drawControlPolygon();
    drawBezierCurve();
    drawControlPoints();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!dragging || selectedPoint < 0 || selectedPoint >= (int)controlPoints.size())
        return;

    vec2 pos = screenToWorld(xpos, ypos);

    pos.x = clamp(pos.x, 0.0f, (float)WINDOW_WIDTH);
    pos.y = clamp(pos.y, 0.0f, (float)WINDOW_HEIGHT);

    controlPoints[selectedPoint] = pos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    vec2 mousePos = screenToWorld(xpos, ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            int idx = findControlPoint(mousePos);

            if (idx != -1)
            {
                selectedPoint = idx;
                dragging = true;
            }
            else
            {
                controlPoints.push_back(mousePos);
                selectedPoint = (int)controlPoints.size() - 1;
                dragging = true;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            dragging = false;
            selectedPoint = -1;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        int idx = findControlPoint(mousePos);
        if (idx != -1)
        {
            controlPoints.erase(controlPoints.begin() + idx);

            if (selectedPoint == idx)
            {
                selectedPoint = -1;
                dragging = false;
            }
            else if (selectedPoint > idx)
            {
                selectedPoint--;
            }
        }
    }
}

int main()
{
    if (!glfwInit())
    {
        cerr << "Hiba: GLFW init sikertelen." << endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Foldesi Kristof - Beadando", nullptr, nullptr);
    if (!window)
    {
        cerr << "Hiba: GLFW ablak letrehozasa sikertelen." << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        cerr << "Hiba: GLEW init sikertelen." << endl;
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    framebuffer_size_callback(window, WINDOW_WIDTH, WINDOW_HEIGHT);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    cout << "Vezerles:" << endl;
    cout << "Bal klikk ures helyre: uj kontrollpont" << endl;
    cout << "Bal klikk es huzas: kontrollpont mozgatasa" << endl;
    cout << "Jobb klikk kontrollponton: kontrollpont torlese" << endl;
    cout << "ESC: kilepes" << endl;

    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}