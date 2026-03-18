#ifndef BALLMANAGER_H
#define BALLMANAGER_H

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
using namespace glm;
#include <cstdlib>
#include <ctime>
#include <vector>
using namespace std;
#include "model.h"
#include "shader.h"
#include "camera.h"

class BallManager {
private:
	vec2 windowSize;

	Model* ball;
	Shader* ballShader;
	Shader* rewardShader;
	GLuint number;						// 当前小球数目
	GLuint gameLevel;					// 游戏难度等级
	GLuint maxNumber;					// 小球最大数目
	GLuint basicNumber;					// 初始小球数目
	vec3 basicPos;						// 小球基础坐标
	vector<vec3> position;				// 场上存在的小球坐标
	vec3 rewardPos;						// 奖励小球坐标
	bool generate_reward;			// 是否生成奖励小球
	float moveSpeed;					// 小球移动速度
	float rewardSpeed;	// 奖励小球移动速度
	GLuint score;						// 得分
	GLuint gameModel;					// 游戏模式
	vec3 lightPos;						// 光源位置
	mat4 lightSpaceMatrix;				// 将顶点世界坐标转换为以光源为中心的坐标
	Texture* rewardTexture;	// 奖励小球纹理
	Camera* camera;
	// 模型变换矩阵
	mat4 model;
	mat4 projection;
	mat4 view;
public:
	BallManager(vec2 windowSize, Camera* camera) {
		this->windowSize = windowSize;
		this->camera = camera;
		basicPos = vec3(0.0, 5.0, -30.0);
		number = 0;
		score = 0;
		this->lightPos = vec3(0.0, 400.0, 150.0);
		mat4 lightProjection = ortho(-100.0f, 100.0f, -100.0f, 100.0f, 1.0f, 500.0f);
		mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0.0, 1.0, 0.0));
		this->lightSpaceMatrix = lightProjection * lightView;
		AddBall();
		LoadModel();
	}
	// 设置游戏模式
	void SetGameModel(GLuint num) {
		gameModel = num;
	}
	void SetBallNumber(GLuint num) {
		basicNumber = num;
		maxNumber = basicNumber;
	}
	void SetGameLevel(GLuint num) {
		gameLevel = num;
		if (gameLevel == 1) {
			moveSpeed = 0.1f;
		}
		else if (gameLevel == 2) {
			moveSpeed = 0.2f;
		}
		else moveSpeed = 0.3f;
	}
	// 更新变换矩阵，判断射击是否击中小球
	void Update(vec3 pos, vec3 dir, bool isShoot) {
		this->view = camera->GetViewMatrix();
		this->projection = perspective(radians(camera->GetZoom()), windowSize.x / windowSize.y, 0.1f, 500.0f);

		if (isShoot) {
			vector<vec3> temp;
			for (GLuint i = 0; i < position.size(); i++) {
				vec3 des = (pos.z - position[i].z) / (-dir.z) * dir + pos;
				if (pow(position[i].x - des.x, 2) + pow(position[i].y - des.y, 2) > 5) {
					temp.push_back(position[i]);
				}
				else {
					number--;
					score++;
				}
			}
			position = temp;
			if (generate_reward) {
				vec3 des = (pos.z - rewardPos.z) / (-dir.z) * dir + pos;
				float away = pow(rewardPos.x - des.x, 2) + pow(rewardPos.y - des.y, 2);
				if (away < 5) {
					score += number;
					generate_reward = false;
					number = 0;
					position.clear();
					if (number == 0) {
						maxNumber++;
						if (maxNumber == basicNumber + 5) {
							moveSpeed += 0.1f;
							maxNumber = basicNumber;
						}
						AddBall();
					}
				}
			}
		}
		if (gameModel == 1) {
			if (number == 0) {
				AddBall();
				return;
			}
			return;
		}
		for (GLuint i = 0; i < position.size(); i++)
			position[i].z += moveSpeed;
		if (generate_reward) {
			rewardSpeed = moveSpeed * 2.0f;
			rewardPos.z += rewardSpeed;
			if (rewardPos.z >= 70) {
				generate_reward = false;
			}
		}
		if (number == 0) {
			maxNumber++;
			if (maxNumber == basicNumber + 5) {
				moveSpeed += 0.1f;
				maxNumber = basicNumber;
			}
			AddBall();
		}
	}
	// 判断游戏是否结束
	bool IsOver() {
		if (position.size() > 0)
			if (position[0].z >= 80)
				return true;
		return false;
	}

	GLuint GetScore() {
		return score;
	}
	// 渲染小球
	void Render(Shader* shader, GLuint depthMap = -1) {
		for (GLuint i = 0; i < position.size(); i++) {
			model = mat4(1.0);
			model[3] = vec4(position[i], 1.0);
			model = scale(model, vec3(5));
			if (shader == NULL) {
				shader = ballShader;
				shader->Bind();
				shader->SetMat4("projection", projection);
				shader->SetMat4("view", view);
			}
			else {
				shader->Bind();
			}
			shader->SetMat4("model", model);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			glBindVertexArray(ball->GetVAO());
			glDrawElements(GL_TRIANGLES, static_cast<GLuint>(ball->GetIndices().size()), GL_UNSIGNED_INT, 0);
			
			shader->Unbind();
			glBindVertexArray(0);
			model = mat4(1.0);
		}
		if (shader == ballShader) shader = NULL; // 重置shader为NULL，避免重复绑定
		if (generate_reward) {
			model = mat4(1.0);
			model[3] = vec4(rewardPos, 1.0);
			model = scale(model, vec3(7.0)); // 增大奖励小球尺寸
			float rotationAngle = glfwGetTime() * 3.0f;
			model = rotate(model, rotationAngle, vec3(0.0f, 1.0f, 0.0f));

			if (shader == NULL) {
				// 正常渲染阶段 - 使用奖励小球着色器
				shader = rewardShader;
				shader->Bind();
				shader->SetMat4("projection", projection);
				shader->SetMat4("view", view);

				// 确保设置使用纹理
				shader->SetBool("useTexture", true);

				// 设置贴图单元
				shader->SetInt("diffuseTexture", 1);
				shader->SetInt("shadowMap", 0);

				// 设置模型矩阵
				shader->SetMat4("model", model);

				// 绑定贴图
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, depthMap);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, rewardTexture->GetId());
			}
			else {
				// 阴影映射阶段 - 使用传入的着色器（通常是深度着色器）
				shader->Bind();

				// 在阴影渲染阶段，我们只关心位置信息，不需要纹理
				// 直接设置模型矩阵即可
				shader->SetMat4("model", model);

				// 这里不需要绑定纹理，因为深度着色器通常不使用颜色信息
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, depthMap);
			}

			// 渲染小球
			glBindVertexArray(ball->GetVAO());
			glDrawElements(GL_TRIANGLES, static_cast<GLuint>(ball->GetIndices().size()), GL_UNSIGNED_INT, 0);
			shader->Unbind();
			glBindVertexArray(0);
			model = mat4(1.0);
		}
	}
private:
	void LoadModel() {
		//普通小球
		ball = new Model("res/model/dot.obj");
		ballShader = new Shader("res/shader/ball.vert", "res/shader/ball.frag");
		ballShader->Bind();
		ballShader->SetVec3("color", vec3(0.8, 1.0, 1.0f));
		ballShader->SetInt("shadowMap", 0);
		ballShader->SetBool("useTexture", false);	
		ballShader->SetVec3("lightPos", lightPos);
		ballShader->SetVec3("viewPos", camera->GetPosition());
		ballShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
		ballShader->Unbind();
		//奖励小球
		rewardShader = new Shader("res/shader/ball.vert", "res/shader/ball.frag");
		rewardShader->Bind();
		rewardShader->SetVec3("color", vec3(1.0, 0.84, 0.0f));
		rewardShader->SetInt("shadowMap", 0);
		rewardShader->SetBool("useTexture", true);
		rewardShader->SetInt("diffuseTexture", 1);
		rewardShader->SetVec3("lightPos", lightPos);
		rewardShader->SetVec3("viewPos", camera->GetPosition());
		rewardShader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
		rewardShader->Unbind();
		rewardTexture = new Texture("res/texture/reward.png");
	}
	// 添加小球
	void AddBall() {
		generate_reward = (rand() % 2 == 0);
		bool flag = generate_reward;
		for (GLuint i = number; i < maxNumber; i++) {
			float judgeX = rand() % 2;
			float x = (judgeX >= 0.5) ? rand() % 30: -(rand() % 30);
			float y = rand() % 30;
			vec3 pos = vec3(basicPos.x + x, basicPos.y + y, basicPos.z);
			if (CheckPosition(pos)) {
				if (flag) {
					rewardPos = pos;
					rewardPos.y += 10.0f;
					flag = false;	
				}
				else {
					position.push_back(pos);
					number++;
				}
			}
			else 
				i--;
		}
	}
	// 检查已存在小球的位置，避免添加的小球出现重叠
	bool CheckPosition(vec3 pos) {
		for (GLuint i = 0; i < position.size(); i++) {
			float away = pow(position[i].x - pos.x, 2) + pow(position[i].y - pos.y, 2);
			if (away < 100)
				return false;
		}
		return true;
	}
};

#endif // !BALLMANAGER_H
