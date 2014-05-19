#define GL_GLEXT_PROTOTYPES

#include "glwidget.h"
#include <QtGui>
#include <GL/gl.h>
#include "GL/glu.h"
#include <math.h>
#include <iostream>

#include "perspectivecamera.h"
#include "orthocamera.h"

GLWidget::GLWidget(QWidget *parent, const QGLWidget *shareWidget)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent, shareWidget),
      tesselationLevel(0),
      zoom(0.0), dragging(false)
{
    scene = NULL;
}

GLWidget::~GLWidget()
{
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 400);
}

void GLWidget::initializeGL()
{
    //Set up OpenGL incantations
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);

    //Create our fbo to hold the rendered scene
    //and the pesky picking buffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    //Create texture to render to
    glGenTextures(1, &renderTex);
    
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 768, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderTex, 0);

    //Now setup the depth buffer
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 768);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);


    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	std::cout << "SOMETHING WENT WRONG IN THE FBO, CHIEF!!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    //Create the drawing quad
    static const GLfloat g_quad_vertex_buffer_data[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f,  1.0f, 0.0f,
	-1.0f,  1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f,  1.0f, 0.0f,
    };
 
    glGenBuffers(1, &canvasQuad);
    glBindBuffer(GL_ARRAY_BUFFER, canvasQuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
}



void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Load the phong shading program
    shaderProgram->bind();
    
    glUniformMatrix4fv(perspectiveMatLocation, 1, GL_FALSE, camera->getProjectionMatrix().constData());

    //Bind the fbo and the textures to draw to
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Draw to the whole texture(the size of the texture, maybe change?)
    glViewport(0,0,1024,768);
    //and set tex 0 as active
    glActiveTexture(GL_TEXTURE0);
    
    if(scene != NULL) {
	//Discombobulate!
	scene->draw(camera->getCameraMatrix());	
    } else {
	std::cout << "no scene yet" << std::endl;
    }

    //Release and relax, brah
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    shaderProgram->release();


    //Now load program to draw to the magic quad
    textureProgram->bind();
    //This time draw to the whole screen
    glViewport(0,0,this->width(), this->height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Make sure the tex 0 is still active
    glActiveTexture(GL_TEXTURE0);
    textureLocation = textureProgram->uniformLocation("renderedTexture");
    //Send the rendered texture down the pipes
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glUniform1i(textureLocation, 0);

    //Draw our nifty, pretty quad
    glBindBuffer(GL_ARRAY_BUFFER, canvasQuad);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, 3*2);
    
    textureProgram->release();
}

void GLWidget::resizeGL(int width, int height)
{
    this->camera->resize(width, height);
    glViewport(0, 0, width, height);
}

void GLWidget::setProjectionLocation(GLuint pL) {
    this->perspectiveMatLocation = pL;
}
void GLWidget::setCamera(Camera *camera) {
    this->camera = camera;
}

void GLWidget::setPerspectiveCamera(double x, double y, double z) {
    this->camera = new PerspectiveCamera(x, y, z, this->width(), this->height());
}

void GLWidget::setOrthoCamera(double x, double y, double z) {
    this->camera = new OrthoCamera(x, y, z, this->width(), this->height());
}

void GLWidget::setScene(Scene *scene) {
    this->scene = scene;
}

void GLWidget::setShaderProgram(QOpenGLShaderProgram *sp) {
    this->shaderProgram = sp;
}
void GLWidget::setTextureProgram(QOpenGLShaderProgram *tp) {
    this->textureProgram = tp;
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton || event->button() == Qt::LeftButton) {
	lastPoint = event->pos();
	dragging = true;
    }
    emit switchActive(this);
}

/*
 * Project coordinates to the surface of the sphere or to
 * the hyperbolic surface, if it doesn's lie on the sphere.
 */
double z(double x, double  y) {
    double length = sqrt(x*x + y*y);
    //Let the radius of the sphere be 1
    if (length <= 1.0/2.0) {
	return sqrt(1.0 - length);
    } else {
	return (1.0/2.0) / sqrt(length);
    }
    
}

double clampUnit(double x) {
    return std::min(1.0, std::max(-1.0, x));
}

QQuaternion M4toQuat(QMatrix4x4 mat) {
    double trace = mat(0,0) + mat(1,1) + mat(2,2) + 1;
    double s = 0.5 / sqrt(trace);
    double w = 0.25 / s;
    double x = (mat(2, 1) - mat(1, 2)) * s;
    double y = (mat(0, 2) - mat(2, 0)) * s;
    double z = (mat(1, 0) - mat(0, 1)) * s;
    return QQuaternion(w, x, y, z);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::RightButton) && dragging) {
	double xtrans = (event->pos().x() - lastPoint.x()) / 1000.0;
	double ytrans = -(event->pos().y() - lastPoint.y()) / 1000.0; //Qt y-coord is inverted
	QVector4D trans(xtrans, ytrans, 0, 1);
	QVector4D worldTrans = trans * camera->getCameraMatrix();
	if(cameraActive) {
	    this->camera->translate(-worldTrans.x(), -worldTrans.y(), -worldTrans.z());
	    updateGL();
	} else {
	    emit translate(worldTrans.x(), worldTrans.y(), worldTrans.z());
	}
    }
    
    if ((event->buttons() & Qt::LeftButton) && dragging) {
	//Here we implement the trackball. Sample two points on the sphere and
	//calculate their angle to use as the rotation.
	
	//normalize to intervals [-1,1]
	double lastx = clampUnit(lastPoint.x() / (this->size().width() / 2.0) - 1.0);
	double lasty = clampUnit(-(lastPoint.y() / (this->size().height() / 2.0) - 1.0));
	
	double newx = clampUnit(event->pos().x() / (this->size().width() / 2.0) - 1.0);
	double newy = clampUnit(-(event->pos().y() / (this->size().height() / 2.0) - 1.0));

	//Project the two points into the sphere (or the hyperbolic plane)
	QVector3D v1(lastx, lasty, z(lastx, lasty));
	v1.normalize();
	QVector3D v2(newx, newy, z(newx, newy));
	v2.normalize();
	
	//Determine the normal of the generated plane through the center of the sphere
	QVector3D normal = QVector3D::crossProduct(v1, v2);
	double theta = acos(QVector3D::dotProduct(v1, v2)) / 3.0;
	
	//angle/2.0, because the quats double cover SO(3)
	QQuaternion newRot = QQuaternion(cos(theta/2.0), sin(theta/2.0) * normal.normalized());
	QQuaternion cameraQuat = M4toQuat(camera->getCameraMatrix());
	QQuaternion worldQuat = cameraQuat.conjugate() * newRot * cameraQuat;
	if(cameraActive) {
	    this->camera->rotate(newRot);
	    updateGL();
	} else {
	    emit rotate(&worldQuat);	    
	}
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton || event->button() == Qt::LeftButton) {
	dragging = false;
    }
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    this->zoom += event->delta()/300.0;
    updateGL();
}

void GLWidget::setTesselation(int tesselationLevel) {
    this->tesselationLevel = tesselationLevel;
    updateGL();
}

void GLWidget::forceGLupdate() {
    updateGL();
}

void GLWidget::resetCamera() {
    if(isActive) {
	this->camera->reset();
	updateGL();
    }
}
