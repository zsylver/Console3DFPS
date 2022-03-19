// Console3DFPS.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>

#define PI 3.141592654f

int nScreenWidth = 240;
int nScreenHeight = 80;

float fPlayerX = 4.f;
float fPlayerY = 4.f;
float fPlayerA = 0.f; // angle / rotational direction player is looking at

int nMapHeight = 16;
int nMapWidth = 16;

float fFOV = PI / 4.0f;
float fDepth = 16.0f;

int main()
{
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	std::wstring map;

	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#......####..###";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"###............#";
	map += L"##.....#########";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#######........#";
	map += L"#..............#";
	map += L"################";

	std::chrono::system_clock::time_point tp1 = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point tp2 = std::chrono::system_clock::now();

	// Game Loop
	while (1)
	{
		// Calculating delta time
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> deltaTime = tp2 - tp1;
		tp1 = tp2;

		float fDeltaTime = deltaTime.count();
		float fSpeedModifier = 8.f;

		// Handle Input
		// Handle Player Rotation
		if (GetAsyncKeyState('A') & 0x8000)
			fPlayerA -= 0.1f * fSpeedModifier * fDeltaTime;

		if (GetAsyncKeyState('D') & 0x8000)
			fPlayerA += 0.1f * fSpeedModifier * fDeltaTime;

		if (GetAsyncKeyState('W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerA) * fSpeedModifier * 0.6f * fDeltaTime;
			fPlayerY += cosf(fPlayerA) * fSpeedModifier * 0.6f * fDeltaTime;

			if (map.c_str()[static_cast<int>(fPlayerY) * nMapWidth + static_cast<int>(fPlayerX)] == '#')
			{
				fPlayerX -= sinf(fPlayerA) * fSpeedModifier * 0.6f * fDeltaTime;
				fPlayerY -= cosf(fPlayerA) * fSpeedModifier * 0.6f * fDeltaTime;
			}
		}

		if (GetAsyncKeyState('S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerA) * fSpeedModifier * 0.6f * fDeltaTime;
			fPlayerY -= cosf(fPlayerA) * fSpeedModifier * 0.6f * fDeltaTime;

			if (map.c_str()[static_cast<int>(fPlayerY) * nMapWidth + static_cast<int>(fPlayerX)] == '#')
			{
				fPlayerX += sinf(fPlayerA) * fSpeedModifier * 0.6f * fDeltaTime;
				fPlayerY += cosf(fPlayerA) * fSpeedModifier * 0.6f * fDeltaTime;
			}
		}


		for (int x{ 0 }; x < nScreenWidth; x++)
		{
			// For each column, calculate the projected ray angle into the world space
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + (x / static_cast<float>(nScreenWidth) * fFOV);

			float fStepSize = 0.1f;			// Increment size for ray casting, decrease to increase
			float fDistanceToWall = 0;

			bool bHitWall = false;			// Set when ray hits wall block
			bool bBoundary = false;			// Set when ray hits boundary between two wall blocks

			float fEyeX = sinf(fRayAngle);	// Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += fStepSize;

				int nTestX = static_cast<int>(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = static_cast<int>(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{
					bHitWall = true;
					fDistanceToWall = fDepth;
				}
				else
				{
					// Ray is inbound so test to see if the ray cell is a wall block
					if (map.c_str()[nTestY * nMapWidth + nTestX] == '#')
					{
						bHitWall = true;

						std::vector<std::pair<float, float>> p; // distance, dot product

						for (int tx{ 0 }; tx < 2; tx++)
							for (int ty{ 0 }; ty < 2; ty++)
							{
								float vy = static_cast<float>(nTestY) + ty - fPlayerY;
								float vx = static_cast<float>(nTestX) + tx - fPlayerX;
								float d = sqrtf(vx * vx + vy * vy); // distance
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(std::make_pair(d, dot));
							}

						// Sort pairs from closest to furthest
						std::sort(p.begin(), p.end(), [](const std::pair<float, float>& lhs, const std::pair<float, float>& rhs) { return lhs.first < rhs.first; });

						float fBound = 0.01f;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor
			int nCeiling = static_cast<int>(nScreenHeight / 2.0f - nScreenHeight / fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';

			if (fDistanceToWall <= fDepth / 4.0f)
				nShade = 0x2588;
			else if (fDistanceToWall < fDepth / 3.0f)
				nShade = 0x2593;
			else if (fDistanceToWall < fDepth / 2.0f)
				nShade = 0x2592;
			else if (fDistanceToWall < fDepth)
				nShade = 0x2591;
			else
				nShade = ' ';

			if (bBoundary) nShade = ' ';

			for (int y{ 0 }; y < nScreenHeight; y++)
			{
				if (y < nCeiling)
					screen[y * nScreenWidth + x] = ' ';
				else if (y > nCeiling && y <= nFloor)
				{
					screen[y * nScreenWidth + x] = nShade;
				}

				else
				{
					// Shade floor based on distance
					float b = 1.f - (y - nScreenHeight / 2.0f) / (nScreenHeight / 2.0f);
					short nShade = ' ';

					if (b < 0.25f)
						nShade = '#';
					else if (b < 0.5f)
						nShade = 'x';
					else if (b < 0.75f)
						nShade = '.';
					else if (b < 0.9f)
						nShade = '-';
					else
						nShade = ' ';

					screen[y * nScreenWidth + x] = nShade;
				}

			}
		}

		// Debugging Info
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f", fPlayerX, fPlayerY, fPlayerA, 1.0f / fDeltaTime);

		// Display Map
		for (int nx{ 0 }; nx < nMapWidth; nx++)
			for (int ny{ 0 }; ny < nMapWidth; ny++)
			{
				screen[(ny + 1) * nScreenWidth + nx] = map.c_str()[ny * nMapWidth + nx];
			}

		screen[(static_cast<int>(fPlayerY) + 1) * nScreenWidth + static_cast<int>(fPlayerX)] = 'P';

		screen[nScreenWidth * nScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	delete[] screen;
	return 0;
}