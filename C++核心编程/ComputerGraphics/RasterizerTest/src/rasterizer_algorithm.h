#pragma once

#include "app_state.h"

float TriangleSignedArea2(const IntPoint triangle[3]);
void myBuildEdgeEquations(const IntPoint& triangle1, const IntPoint& triangle2, EdgeEquation& edge);
void BuildEdgeEquations(const IntPoint triangle[3], EdgeEquation edges[3]);
bool IsDegenerateTriangle(const IntPoint triangle[3]);
BoundingBox CalculateBoundingBox(const IntPoint triangle[3]);
bool EvaluateInside(const EdgeEquation edges[3], float x, float y, float values[3]);
bool EvaluateInside(const EdgeEquation edges[3], float x, float y);
FloatColor InterpolateColor(
    const IntPoint triangle[3],
    const FloatColor vertexColors[3],
    float x,
    float y);
bool ArePointsCoincident(const IntPoint triangle[3]);
bool ArePointsCollinear(const IntPoint triangle[3]);