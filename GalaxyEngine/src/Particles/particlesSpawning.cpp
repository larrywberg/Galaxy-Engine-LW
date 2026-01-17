#include "Particles/particlesSpawning.h"

#include "Physics/physics.h"
#include "Physics/quadtree.h"

#include "parameters.h"

void ParticlesSpawning::particlesInitialConditions(Physics& physics, UpdateVariables& myVar, UpdateParameters& myParam) {

	if (myVar.isMouseNotHoveringUI && isSpawningAllowed) {

		Slingshot slingshot = slingshot.particleSlingshot(myVar, myParam.myCamera);
		const auto randFloat = []() -> float {
			return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		};

		if (myVar.isDragging && enablePathPrediction && myVar.gridExists) {
			predictTrajectory(myParam.pParticles, myParam.myCamera, physics, myVar, slingshot);
		}

		if (IO::mouseReleased(0) && myVar.toolSpawnHeavyParticle && !IO::shortcutDown(KEY_LEFT_CONTROL) && !IO::shortcutDown(KEY_LEFT_ALT) && myVar.isDragging) {
			
			myParam.pParticles.emplace_back(
				myParam.myCamera.mouseWorldPos,
				slingshot.norm * slingshot.length,
				heavyParticleInitMass * heavyParticleWeightMultiplier,

				0.008f,
				1.0f,
				1.0f,
				1.0f
			);
			myParam.rParticles.emplace_back(
				Color{ 255, 255, 255, 255 },
				0.3f,
				true,
				false,
				true,
				false,
				false,
				false,
				false,
				-1.0f,
				0
			);
			myVar.isDragging = false;
		}

		if (!myVar.isSPHEnabled) {
			myVar.isBrushDrawing = false;
			myVar.constraintAfterDrawingFlag = false;
		}

		if ((IO::mouseDown(2) || (IO::mouseDown(0) && myVar.toolDrawParticles)) && !IO::shortcutDown(KEY_LEFT_CONTROL) && !IO::shortcutDown(KEY_LEFT_ALT) && !IO::shortcutDown(KEY_X)) {
			myVar.isBrushDrawing = true;

			myParam.brush.brushLogic(myParam, myVar.isSPHEnabled, myVar.constraintAfterDrawing);
			if (myVar.isBrushDrawing) {
				if (myVar.isSPHEnabled) {

					particlesIterating = true;

#pragma omp parallel for
					for (int i = 0; i < correctionSubsteps; i++) {
						physics.buildGrid(myParam.pParticles, myParam.rParticles, physics, myVar.domainSize, correctionSubsteps);
					}
				}
			}
		}
		else {

			if (myVar.isSPHEnabled && myVar.isBrushDrawing) {

				particlesIterating = false;
				for (size_t i = 0; i < myParam.pParticles.size(); i++) {
					if (myParam.rParticles[i].spawnCorrectIter < correctionSubsteps && myParam.rParticles[i].isBeingDrawn) {
						particlesIterating = true;
						break;
					}
				}

				if (particlesIterating) {
#pragma omp parallel for
					for (int i = 0; i < correctionSubsteps * 2; i++) {
						physics.buildGrid(myParam.pParticles, myParam.rParticles, physics, myVar.domainSize, correctionSubsteps);
					}
				}
				else {

					if (myVar.constraintAfterDrawing) {
						myVar.constraintAfterDrawingFlag = true;
					}

					if (myVar.constraintAfterDrawingFlag && myVar.constraintAfterDrawing) {
						physics.createConstraints(myParam.pParticles, myParam.rParticles, 
							myVar.constraintAfterDrawingFlag, myVar);
					}

					for (size_t i = 0; i < myParam.pParticles.size(); i++) {
						myParam.rParticles[i].isBeingDrawn = false;
					}
					myVar.isBrushDrawing = false;
				}
			}
		}

		if (myVar.isBrushDrawing) {
			if (!myVar.autoPausedForBrush && myVar.isTimePlaying) {
				myVar.wasTimePlayingBeforeBrush = true;
				myVar.autoPausedForBrush = true;
				myVar.isTimePlaying = false;
			}

			if (myVar.autoPausedForBrush) {
				myVar.timeFactor = 0.0f;
			}
		}
		else if (myVar.autoPausedForBrush) {
			myVar.isTimePlaying = myVar.wasTimePlayingBeforeBrush;
			myVar.autoPausedForBrush = false;
			myVar.wasTimePlayingBeforeBrush = false;
		}


		if ((IO::shortcutReleased(KEY_ONE) || IO::mouseReleased(0) && myVar.toolSpawnBigGalaxy) && myVar.isDragging) {

			// VISIBLE MATTER

			const glm::vec2 galaxyCenter = myParam.myCamera.mouseWorldPos;
			const float outerRadius = 200.0f;
			const float scaleLength = 90.0f;
			const int clusterCount = 40;
			const float clusterRadiusMin = 6.0f;
			const float clusterRadiusMax = 24.0f;
			const float clusterBlend = 0.35f;

			std::vector<glm::vec2> clusterCenters;
			std::vector<float> clusterRadii;
			clusterCenters.reserve(clusterCount);
			clusterRadii.reserve(clusterCount);

			for (int c = 0; c < clusterCount; ++c) {
				float normalizedRand = randFloat();
				float angle = randFloat() * 2.0f * PI;
				float finalRadius = -scaleLength * log(1.0f - normalizedRand);
				finalRadius = std::min(finalRadius, outerRadius + 600.0f);
				finalRadius = std::max(finalRadius, 0.01f);
				glm::vec2 center = glm::vec2(
					galaxyCenter.x + finalRadius * cos(angle),
					galaxyCenter.y + finalRadius * sin(angle)
				);
				clusterCenters.push_back(center);
				clusterRadii.push_back(clusterRadiusMin + randFloat() * (clusterRadiusMax - clusterRadiusMin));
			}

			for (int i = 0; i < static_cast<int>(40000 * particleAmountMultiplier); i++) {
				const float normalizedRand = randFloat();
				const float angle = randFloat() * 2.0f * PI;
				float diskRadius = -scaleLength * log(1.0f - normalizedRand);
				diskRadius = std::min(diskRadius, outerRadius + 600.0f);
				diskRadius = std::max(diskRadius, 0.01f);
				const glm::vec2 basePos = glm::vec2(
					galaxyCenter.x + diskRadius * cos(angle),
					galaxyCenter.y + diskRadius * sin(angle)
				);

				const int clusterIndex = rand() % clusterCount;
				const float localAngle = randFloat() * 2.0f * PI;
				const float localRadius = std::sqrt(randFloat()) * clusterRadii[clusterIndex];
				const glm::vec2 localOffset = {
					localRadius * cos(localAngle),
					localRadius * sin(localAngle)
				};

				const glm::vec2 clusterPos = clusterCenters[clusterIndex] + localOffset;
				glm::vec2 pos = basePos * (1.0f - clusterBlend) + clusterPos * clusterBlend;

				glm::vec2 dGlobal = pos - galaxyCenter;
				float globalRadius = sqrt(dGlobal.x * dGlobal.x + dGlobal.y * dGlobal.y);
				globalRadius = std::max(globalRadius, 0.01f);
				glm::vec2 tangentGlobal = glm::vec2(dGlobal.y, -dGlobal.x) / globalRadius;
				float globalSpeed = 10.5f * sqrt(1758.0f / (globalRadius + 54.7f));

				glm::vec2 dLocal = pos - clusterCenters[clusterIndex];
				float localOrbitRadius = sqrt(dLocal.x * dLocal.x + dLocal.y * dLocal.y);
				localOrbitRadius = std::max(localOrbitRadius, 1.0f);
				glm::vec2 tangentLocal = glm::vec2(dLocal.y, -dLocal.x) / localOrbitRadius;
				float localSpeed = 4.5f * sqrt(200.0f / (localOrbitRadius + 12.0f));
				localSpeed *= clusterBlend;

				glm::vec2 vel = tangentGlobal * globalSpeed + tangentLocal * localSpeed;

				float finalMass = 0.0f;

				if (massMultiplierEnabled) {
					finalMass = 8500000000.0f / particleAmountMultiplier;
				}
				else {
					finalMass = 8500000000.0f;
				}

				myParam.pParticles.emplace_back(
					pos,
					vel + slingshot.norm * slingshot.length * 0.3f,
					finalMass,

					0.008f,
					1.0f,
					1.0f,
					1.0f
				);
				myParam.rParticles.emplace_back(
					Color{ 128, 128, 128, 100 },
					0.125f,
					false,
					false,
					false,
					true,
					true,
					false,
					true,
					-1.0f,
					0
				);
			}

			// DARK MATTER

			if (myVar.isDarkMatterEnabled) {
				for (int i = 0; i < static_cast<int>(12000 * DMAmountMultiplier); i++) {
					glm::vec2 galaxyCenter = myParam.myCamera.mouseWorldPos;

					float outerRadius = 2000.0f;
					float radiusCore = 3.5f;

					float normalizedRand = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

					float radiusMultiplier = radiusCore * sqrt(static_cast<float>(pow(1 + pow(outerRadius / radiusCore, 2), normalizedRand) - 1));

					float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2 * PI;

					glm::vec2 pos = glm::vec2(galaxyCenter.x + radiusMultiplier * cos(angle), galaxyCenter.y + radiusMultiplier * sin(angle));

					glm::vec2 vel = glm::vec2(static_cast<float>(rand() % 60 - 30), static_cast<float>(rand() % 60 - 30));

					float finalMass = 0.0f;

					if (massMultiplierEnabled) {
						finalMass = 141600000000.0f / DMAmountMultiplier;
					}
					else {
						finalMass = 141600000000.0f;
					}

					myParam.pParticles.emplace_back(
						pos,
						vel + slingshot.norm * slingshot.length * 0.3f,
						finalMass,

						0.008f,
						1.0f,
						1.0f,
						1.0f
					);
					myParam.rParticles.emplace_back(
						Color{ 128, 128, 128, 0 },
						0.125f,
						true,
						false,
						false,
						false,
						true,
						true,
						false,
						-1.0f,
						0
					);
				}
			}

			myVar.isDragging = false;
		}

		if ((IO::shortcutReleased(KEY_TWO) || IO::mouseReleased(0) && myVar.toolSpawnSmallGalaxy) && myVar.isDragging) {

			// VISIBLE MATTER

			const glm::vec2 galaxyCenter = myParam.myCamera.mouseWorldPos;
			const float outerRadius = 100.0f;
			const float scaleLength = 45.0f;
			const int clusterCount = 20;
			const float clusterRadiusMin = 5.0f;
			const float clusterRadiusMax = 18.0f;
			const float clusterBlend = 0.4f;

			std::vector<glm::vec2> clusterCenters;
			std::vector<float> clusterRadii;
			clusterCenters.reserve(clusterCount);
			clusterRadii.reserve(clusterCount);

			for (int c = 0; c < clusterCount; ++c) {
				float normalizedRand = randFloat();
				float angle = randFloat() * 2.0f * PI;
				float finalRadius = -scaleLength * log(1.0f - normalizedRand);
				finalRadius = std::min(finalRadius, outerRadius + 300.0f);
				finalRadius = std::max(finalRadius, 0.01f);
				glm::vec2 center = glm::vec2(
					galaxyCenter.x + finalRadius * cos(angle),
					galaxyCenter.y + finalRadius * sin(angle)
				);
				clusterCenters.push_back(center);
				clusterRadii.push_back(clusterRadiusMin + randFloat() * (clusterRadiusMax - clusterRadiusMin));
			}

			for (int i = 0; i < static_cast<int>(12000 * particleAmountMultiplier); i++) {
				const float normalizedRand = randFloat();
				const float angle = randFloat() * 2.0f * PI;
				float diskRadius = -scaleLength * log(1.0f - normalizedRand);
				diskRadius = std::min(diskRadius, outerRadius + 300.0f);
				diskRadius = std::max(diskRadius, 0.01f);
				const glm::vec2 basePos = glm::vec2(
					galaxyCenter.x + diskRadius * cos(angle),
					galaxyCenter.y + diskRadius * sin(angle)
				);

				const int clusterIndex = rand() % clusterCount;
				const float localAngle = randFloat() * 2.0f * PI;
				const float localRadius = std::sqrt(randFloat()) * clusterRadii[clusterIndex];
				const glm::vec2 localOffset = {
					localRadius * cos(localAngle),
					localRadius * sin(localAngle)
				};

				const glm::vec2 clusterPos = clusterCenters[clusterIndex] + localOffset;
				glm::vec2 pos = basePos * (1.0f - clusterBlend) + clusterPos * clusterBlend;

				glm::vec2 dGlobal = pos - galaxyCenter;
				float globalRadius = sqrt(dGlobal.x * dGlobal.x + dGlobal.y * dGlobal.y);
				globalRadius = std::max(globalRadius, 0.01f);
				glm::vec2 tangentGlobal = glm::vec2(dGlobal.y, -dGlobal.x) / globalRadius;
				float globalSpeed = 10.5f * sqrt(505.0f / (globalRadius + 54.7f));

				glm::vec2 dLocal = pos - clusterCenters[clusterIndex];
				float localOrbitRadius = sqrt(dLocal.x * dLocal.x + dLocal.y * dLocal.y);
				localOrbitRadius = std::max(localOrbitRadius, 1.0f);
				glm::vec2 tangentLocal = glm::vec2(dLocal.y, -dLocal.x) / localOrbitRadius;
				float localSpeed = 4.0f * sqrt(120.0f / (localOrbitRadius + 10.0f));
				localSpeed *= clusterBlend;

				glm::vec2 vel = tangentGlobal * globalSpeed + tangentLocal * localSpeed;

				float finalMass = 0.0f;

				if (massMultiplierEnabled) {
					finalMass = 8500000000.0f / particleAmountMultiplier;
				}
				else {
					finalMass = 8500000000.0f;
				}

				myParam.pParticles.emplace_back(
					pos,
					vel + slingshot.norm * slingshot.length * 0.3f,
					finalMass,

					0.008f,
					1.0f,
					1.0f,
					1.0f
				);
				myParam.rParticles.emplace_back(
					Color{ 128, 128, 128, 100 },
					0.125f,
					false,
					false,
					false,
					true,
					true,
					false,
					true,
					-1.0f,
					0
				);
			}

			// DARK MATTER
			if (myVar.isDarkMatterEnabled) {
				for (int i = 0; i < static_cast<int>(3600 * DMAmountMultiplier); i++) {
					glm::vec2 galaxyCenter = myParam.myCamera.mouseWorldPos;

					float outerRadius = 2000.0f;
					float radiusCore = 3.5f;

					float normalizedRand = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

					float radiusMultiplier = radiusCore * sqrt(static_cast<float>(pow(1 + pow(outerRadius / radiusCore, 2), normalizedRand) - 1));

					float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2 * PI;

					glm::vec2 pos = glm::vec2(galaxyCenter.x + radiusMultiplier * cos(angle), galaxyCenter.y + radiusMultiplier * sin(angle));

					glm::vec2 vel = glm::vec2(static_cast<float>(rand() % 60 - 30), static_cast<float>(rand() % 60 - 30));

					float finalMass = 0.0f;

					if (massMultiplierEnabled) {
						finalMass = 141600000000.0f / DMAmountMultiplier;
					}
					else {
						finalMass = 141600000000.0f;
					}

					myParam.pParticles.emplace_back(
						pos,
						vel + slingshot.norm * slingshot.length * 0.3f,
						finalMass,

						0.008f,
						1.0f,
						1.0f,
						1.0f
					);
					myParam.rParticles.emplace_back(
						Color{ 128, 128, 128, 0 },
						0.125f,
						true,
						false,
						false,
						false,
						true,
						true,
						false,
						-1.0f,
						0
					);
				}
			}
			myVar.isDragging = false;
		}

		if ((IO::shortcutReleased(KEY_THREE) || IO::mouseReleased(0) && myVar.toolSpawnStar) && !IO::shortcutDown(KEY_LEFT_CONTROL) && !IO::shortcutDown(KEY_LEFT_ALT)) {

			for (int i = 0; i < static_cast<int>(10000 * particleAmountMultiplier); i++) {

				float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159f;
				float distance = (sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 5.0f) + 0.1f;

				glm::vec2 randomOffset = {
					cos(angle) * distance,
					sin(angle) * distance
				};

				glm::vec2 particlePos = myParam.myCamera.mouseWorldPos + randomOffset;

				float finalMass = 0.0f;

				if (massMultiplierEnabled) {
					finalMass = 8500000000.0f / particleAmountMultiplier;
				}
				else {
					finalMass = 8500000000.0f;
				}

				myParam.pParticles.emplace_back(
					glm::vec2{ particlePos.x, particlePos.y },
					slingshot.norm * slingshot.length * 0.3f,
					finalMass,

					0.008f,
					1.0f,
					1.0f,
					1.0f
				);
				myParam.rParticles.emplace_back(
					Color{ 128, 128, 128, 100 },
					0.125f,
					false,
					false,
					false,
					true,
					true,
					false,
					true,
					-1.0f,
					0
				);
			}

			myVar.isDragging = false;
		}

		if ((IO::shortcutPress(KEY_FOUR) || IO::mouseReleased(0) && myVar.toolSpawnBigBang) && !IO::shortcutDown(KEY_LEFT_CONTROL) && !IO::shortcutDown(KEY_LEFT_ALT)) {

			// VISIBLE MATTER

			for (int i = 0; i < static_cast<int>(40000 * particleAmountMultiplier); i++) {
				float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159f;
				float distance = (sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 20.0f) + 1.0f;

				glm::vec2 randomOffset = {
					cos(angle) * distance + static_cast<float>(GetRandomValue(-15, 15)),
					sin(angle) * distance + static_cast<float>(GetRandomValue(-15, 15))
				};

				glm::vec2 particlePos = myParam.myCamera.mouseWorldPos + randomOffset;

				glm::vec2 d = particlePos - myParam.myCamera.mouseWorldPos;

				glm::vec2 norm = d / distance;

				float speed = 300.0f;

				float adjustedSpeed = speed * (distance / 35.0f);

				glm::vec2 vel = adjustedSpeed * norm;

				float finalMass = 0.0f;

				if (massMultiplierEnabled) {
					finalMass = 8500000000.0f / particleAmountMultiplier;
				}
				else {
					finalMass = 8500000000.0f;
				}

				myParam.pParticles.emplace_back(
					particlePos,
					vel,
					finalMass,

					0.008f,
					1.0f,
					1.0f,
					1.0f
				);
				myParam.rParticles.emplace_back(
					Color{ 128, 128, 128, 100 },
					0.125f,
					false,
					false,
					false,
					true,
					true,
					false,
					true,
					-1.0f,
					0
				);
			}

			// DARK MATTER

			if (myVar.isDarkMatterEnabled) {
				for (int i = 0; i < static_cast<int>(12000 * DMAmountMultiplier); i++) {

					float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159f;
					float distance = (sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 20.0f) + 1.0f;

					glm::vec2 randomOffset = {
						cos(angle) * distance + static_cast<float>(GetRandomValue(-15, 15)),
						sin(angle) * distance + static_cast<float>(GetRandomValue(-15, 15))
					};

					glm::vec2 particlePos = myParam.myCamera.mouseWorldPos + randomOffset;

					glm::vec2 d = particlePos - myParam.myCamera.mouseWorldPos;

					glm::vec2 norm = d / distance;

					float speed = 300.0f;

					float adjustedSpeed = speed * (distance / 35.0f);

					glm::vec2 vel = adjustedSpeed * norm;

					float finalMass = 0.0f;

					if (massMultiplierEnabled) {
						finalMass = 141600000000.0f / DMAmountMultiplier;
					}
					else {
						finalMass = 141600000000.0f;
					}

					myParam.pParticles.emplace_back(
						particlePos,
						vel,
						finalMass,

						0.008f,
						1.0f,
						1.0f,
						1.0f
					);
					myParam.rParticles.emplace_back(
						Color{ 128, 128, 128, 0 },
						0.125f,
						true,
						false,
						false,
						false,
						true,
						true,
						false,
						-1.0f,
						0
					);
				}
			}
		}
	}
	else {
		if (IsMouseButtonPressed(0)) {
			isSpawningAllowed = false;
		}
	}

	if (IsMouseButtonReleased(0)) {
		isSpawningAllowed = true;
		myVar.isDragging = false;
	}
}

void ParticlesSpawning::predictTrajectory(const std::vector<ParticlePhysics>& pParticles,
	SceneCamera& myCamera, Physics physics,
	UpdateVariables& myVar, Slingshot& slingshot) {

	if (!IsMouseButtonDown(0)) {
		return;
	}

	std::vector<ParticlePhysics> currentParticles = pParticles;

	ParticlePhysics predictedParticle(
		glm::vec2(myCamera.mouseWorldPos),
		glm::vec2{ slingshot.norm * slingshot.length },
		heavyParticleInitMass * heavyParticleWeightMultiplier,

		1.0f,
		1.0f,
		1.0f,
		1.0f
	);

	currentParticles.push_back(predictedParticle);

	int predictedIndex = static_cast<int>(currentParticles.size()) - 1;

	std::vector<glm::vec2> predictedPath;

	for (int step = 0; step < predictPathLength; ++step) {
		ParticlePhysics& p = currentParticles[predictedIndex];

		glm::vec2 netForce = physics.calculateForceFromGrid(currentParticles, myVar, p);

		glm::vec2 acc;
		acc = netForce / p.mass;

		p.vel += myVar.timeFactor * (1.5f * acc);

		p.pos += p.vel * myVar.timeFactor;

		predictedPath.push_back(p.pos);
	}

	for (size_t i = 1; i < predictedPath.size(); ++i) {
		DrawLineV({ predictedPath[i - 1].x,  predictedPath[i - 1].y }, { predictedPath[i].x,  predictedPath[i].y }, WHITE);
	}
}
