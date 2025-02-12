#ifndef GRID_H
#define GRID_H

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

struct Particle
{
	glm::vec2 position;
	glm::vec2 velocity;
};

class Cell {
public:
	Particle* particles[2];
	int count;

	Cell() : count(0) {}

	void addParticle(Particle* p)
	{
		if (count < 2) {
			particles[count++] = p;
		}
		else {
			std::cerr << "Cannot add more particles to the cell" << std::endl;
		}
	}
};

class Grid {

public:
	Grid(int width, int height)
		: width(width), height(height)
	{
		cells = new Cell[width * height];
	}

	~Grid() {
		delete[] cells;
	}

	void insertParticle(Particle &p)
	{
		int cellX = static_cast<int>(p.position.x);
		int cellY = static_cast<int>(p.position.y);
		int cellIndex = cellY * width + cellX;
		cells[cellIndex].addParticle(&p);
	}

	void detectCollisions()
	{
		for (int i = 0; i < width * height; i++) {
			Cell& cell = cells[i];
			for (int j = 0; j < cell.count; ++j) {
				for (int k = j + 1; k < cell.count; ++k) {
					resolveCollision(*cell.particles[j], *cell.particles[k]);
				}
			}
		}
	}

private:
	int width, height;
	Cell* cells;

	void resolveCollision(Particle &p1, Particle &p2)
	{
		glm::vec2 delta = p1.position - p2.position;
		float distance = glm::length(delta);
		if (distance < 0.06f)
		{
			std::swap(p1.velocity, p2.velocity);
		}
	}
};

#endif