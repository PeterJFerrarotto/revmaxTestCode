#ifndef _SIMULATOR_H
#define _SIMULATOR_H
#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include "RequestManager.h"
#include "RideRequest.h"
#include "Vehicle.h"
#include "dirent.h"
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "ShaderProgram.h"
#include <SDL.h>
#include <vector>
#include <iostream>
#include "renderingMathHelper.h"
#include "Texture.h"
#include "Button.h"
#include <future>
#include <chrono>
#include "BasicExcel.hpp"
using namespace YExcel;

using namespace rapidxml;

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64) || defined(WIN82) || defined(_WIN82)
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define FIXED_TIMESTEP 1/30.0f
#define MAX_TIMESTEP 6
#define FRAMES_PER_SECOND 6.0f

class Simulator{
protected:
	std::unordered_map<std::string, std::vector<Vehicle*>> vehicles;
	std::unordered_map<std::string, RequestManager*> managers;
	std::vector<std::string> tests;
	std::string currentTest;
	std::string excelFileName;
	bool runningRanged;

	std::unordered_map<std::string, float> weightOfDistanceOfTrip;
	std::unordered_map<std::string, unsigned> timesToRun;
	std::unordered_map<std::string, float> radiusMin;
	std::unordered_map<std::string, float> radiusStep;
	std::unordered_map<std::string, float> radiusMax;
	std::unordered_map<std::string, float> timeRadius;
	std::unordered_map<std::string, float> minimumScore;
	std::unordered_map<std::string, int> maxRideRequests;
	std::map<int, float> results;
	std::map<int, int> totalDistanceWithPassenger;
	std::map<int, int> totalDistanceWithoutPassenger;
	std::map<int, int> numberOfCompletedRequests;

	Texture* lineTexture;
	Texture* requestTexture;
	Texture* vehicleTexture;
	Texture* destinationTexture;
	Texture* textSheet;
	Texture* gridTexture;

	float offsetX, offsetY;
	float scaleOffsetX, scaleOffsetY;
	float windowSizeOffsetX, windowSizeOffsetY;
	float simTime;
	inline xml_document<> * loadXMLFile(const char* filePath){
		file<>* xmlFile = new file<>(filePath);
		xml_document<>* doc = new xml_document<>;
		doc->parse<0>(xmlFile->data());
		return doc;
	}

	inline float scoreRequest(Vehicle* vehicle, RideRequest* request, RequestManager* manager, int time, int timeRadius, float weightOfDistanceOfRide, int maxRideRequests, float(*routing)(float, float, float, float)){
		float distanceToRide = routing(vehicle->getCurrentLocation().first, vehicle->getCurrentLocation().second, request->getLocation().first, request->getLocation().second);
		//Assumption: each mile is travelled in one hour.
		//When travelling by routing in a city, what is a short distance suddenly becomes an incredibly long distance.
		int timeToUse = time;
		if (vehicle->getTopRequest() != nullptr){
			timeToUse = vehicle->getTopRequest()->getRequestTime() + vehicle->getTopRequest()->getDistanceOfRequest();
		}
		if (request->getRequestTime() > timeToUse + ceil(distanceToRide) + timeRadius || request->getRequestTime() < timeToUse + ceil(distanceToRide)){
			return -1;
		}
		float distanceOfRide = request->getDistanceOfRequestCalculated() ? request->getDistanceOfRequest() : routing(request->getLocation().first, request->getLocation().second, request->getDestination().first, request->getDestination().second);
		//Currently using the calculated distance as the time.
		int timeOfRide = (int)distanceOfRide;
		int numOfRequestsAtDestination = manager->getNumberOfRequestsAtLocation(request->getDestination(), request->getRequestTime() + timeOfRide, timeRadius);
		float score;
		float percentageUtilization;
		float rideDistanceValue;
		float destinationPenalty = 0;

		//The smaller the distance to the ride, the greater the score.
		percentageUtilization = (distanceOfRide / (distanceToRide + distanceOfRide)) * 10;

		request->setDistanceToRequest(ceil(distanceToRide));
		request->setDistanceOfRequest(distanceOfRide);
		request->setRequestsAtDestination(numOfRequestsAtDestination);
		rideDistanceValue = ((distanceOfRide)* (weightOfDistanceOfRide)) * 10;

		if (numOfRequestsAtDestination < maxRideRequests){
			destinationPenalty = 3 - (numOfRequestsAtDestination * (3 / maxRideRequests));
		}

		score = percentageUtilization + rideDistanceValue - destinationPenalty;
		return score * 10;
	}

	inline void enrichRequestData(xml_node<>* requestNode){
		float locLat, locLong;
		float destLat, destLong;
		int time;

		if (requestNode->first_node("Location") == nullptr){
			throw "No request location!";
		}
		xml_node<>* locationNode = requestNode->first_node("Location");
		if (locationNode->first_attribute("long") == nullptr || locationNode->first_attribute("lat") == nullptr){
			throw "No latitude or longitude!";
		}
		locLat = std::stof(locationNode->first_attribute("lat")->value());
		locLong = std::stof(locationNode->first_attribute("long")->value());

		if (requestNode->first_node("Destination") == nullptr){
			throw "No request destination!";
		}
		xml_node<>* destinationNode = requestNode->first_node("Destination");
		if (destinationNode->first_attribute("long") == nullptr || destinationNode->first_attribute("lat") == nullptr){
			throw "No latitude or longitude!";
		}
		destLat = std::stof(destinationNode->first_attribute("lat")->value());
		destLong = std::stof(destinationNode->first_attribute("long")->value());

		if (requestNode->first_node("RequestTime") == nullptr){
			throw "No time specified for request!";
		}
		time = std::stoi(requestNode->first_node("RequestTime")->value());
		RideRequest* request = new RideRequest;
		request->setLocation(locLat, locLong);
		request->setDestination(destLat, destLong);
		request->setRequestTime(time);
		managers[currentTest]->addRequest(request);
	}

	inline void createRandomRequest(float minLat, float minLong, float maxLat, float maxLong){
		float locLat, locLong;
		float destLat, destLong;
		int time;
		locLat = randomRangedLong(minLat, maxLat);
		locLong = randomRangedLong(minLong, maxLong);
		destLat = randomRangedLong(minLat, maxLat);
		destLong = randomRangedLong(minLong, maxLong);
		time = randomRangedInt(0, timesToRun[currentTest]);

		RideRequest* request = new RideRequest;
		request->setLocation(locLat, locLong);
		request->setDestination(destLat, destLong);
		request->setRequestTime(time);
		managers[currentTest]->addRequest(request);
	}

	inline void enrichVehicleData(xml_node<>* vehicleNode){
		float lat, longitude;
		if (vehicleNode->first_node("Location") == nullptr){
			throw "No location!";
		}

		xml_node<>* currentNode = vehicleNode->first_node("Location");
		if (currentNode->first_attribute("long") == nullptr || currentNode->first_attribute("lat") == nullptr){
			throw "No latitude or longitude!";
		}

		lat = std::stof(currentNode->first_attribute("lat")->value());
		longitude = std::stof(currentNode->first_attribute("long")->value());

		Vehicle* vehicle = new Vehicle;
		vehicle->setStartingLocation(lat, longitude);
		vehicles[currentTest].push_back(vehicle);
	}

	inline void createRandomVehicle(float minLat, float minLong, float maxLat, float maxLong){
		float locLat, locLong;
		locLat = randomRangedLong(minLat, maxLat);
		locLong = randomRangedLong(minLong, maxLong);
		//locLat = randomRangedInt(minLat, maxLat);
		//locLong = randomRangedLong(minLong, maxLong);
		Vehicle* vehicle = new Vehicle;
		vehicle->setStartingLocation(locLat, locLong);
		vehicles[currentTest].push_back(vehicle);
	}

	inline void enrichManagerData(xml_node<>* managerNode){
		int minLat = 0, minLong = 0;
		int maxLat = 20, maxLong = 20;
		int sectionSize = 5;

		if (managerNode->first_attribute("minLat") != nullptr){
			minLat = std::stoi(managerNode->first_attribute("minLat")->value());
		}

		if (managerNode->first_attribute("minLong") != nullptr){
			minLong = std::stoi(managerNode->first_attribute("minLong")->value());
		}

		if (managerNode->first_attribute("maxLat") != nullptr){
			maxLat = std::stoi(managerNode->first_attribute("maxLat")->value());
		}

		if (managerNode->first_attribute("maxLong") != nullptr){
			maxLong = std::stoi(managerNode->first_attribute("maxLong")->value());
		}

		if (managerNode->first_attribute("sectionSize") != nullptr){
			sectionSize = std::stoi(managerNode->first_attribute("sectionSize")->value());
		}

		managers[currentTest] = new RequestManager();

		managers[currentTest]->setLatitudeMin(minLat);
		managers[currentTest]->setLongitudeMin(minLong);
		managers[currentTest]->setLatitudeMax(maxLat);
		managers[currentTest]->setLongitudeMax(maxLong);
		managers[currentTest]->setSectionRadius(sectionSize);
		managers[currentTest]->initializeRequestMap();

		managers[currentTest]->setLineTexture(lineTexture);
		managers[currentTest]->setGridTexture(gridTexture);
		managers[currentTest]->setRequestTexture(requestTexture);
		//managers[currentTest]->setVenueTexture(venueTexture);
		managers[currentTest]->setDestinationTexture(destinationTexture);
	}

	inline void createManagerFromParams(float maxLong, float maxLat, float sectionSize){
		managers[currentTest] = new RequestManager();
		managers[currentTest]->setLatitudeMin(0);
		managers[currentTest]->setLongitudeMin(0);
		managers[currentTest]->setLatitudeMax(maxLat);
		managers[currentTest]->setLongitudeMax(maxLong);
		managers[currentTest]->setSectionRadius(sectionSize);
		managers[currentTest]->initializeRequestMap();

		managers[currentTest]->setLineTexture(lineTexture);
		managers[currentTest]->setGridTexture(gridTexture);
		managers[currentTest]->setRequestTexture(requestTexture);
		//managers[currentTest]->setVenueTexture(venueTexture);
		managers[currentTest]->setDestinationTexture(destinationTexture);
	}

	inline void initialize(const std::string& customTestName, unsigned customTimesToRun, float customTripWeight, float customRadiusMin, float customRadiusStep,
		float customRadiusMax, float customTimeRadius, float customMinimumScore, unsigned customMaximumRideRequests, unsigned customFleetSize, unsigned customRideCount,
		float customMaxLat, float customMaxLong, float customSectionSize, bool ranged){
		runningRanged = ranged;
		currentTest = customTestName;
		tests.push_back(currentTest);
		weightOfDistanceOfTrip[currentTest] = customTripWeight;
		timesToRun[currentTest] = customTimesToRun;
		radiusMin[currentTest] = customRadiusMin;
		radiusStep[currentTest] = customRadiusStep;
		radiusMax[currentTest] = customRadiusMax;
		timeRadius[currentTest] = customTimeRadius;
		minimumScore[currentTest] = customMinimumScore;
		maxRideRequests[currentTest] = customMaximumRideRequests;
		createManagerFromParams(customMaxLong, customMaxLat, customSectionSize);
		if (!ranged || (ranged && tests.size() == 1)){
			for (int i = 0; i < customFleetSize; i++){
				createRandomVehicle(0, 0, customMaxLat, customMaxLong);
			}
			for (int i = 0; i < customRideCount; i++){
				createRandomRequest(0, 0, customMaxLat, customMaxLong);
			}
		}
		else if (ranged && tests.size() > 1){
			for (int i = 0; i < customFleetSize; i++){
				Vehicle* vehicle = new Vehicle();
				vehicle->setStartingLocation(vehicles[tests[0]][i]->getCurrentLocation().first, vehicles[tests[0]][i]->getCurrentLocation().second);
				vehicles[currentTest].push_back(vehicle);
			}
			std::vector<RideRequest*> requests = managers[tests[0]]->getAllRideRequests();
			for (int i = 0; i < customRideCount; i++){
				RideRequest* request = new RideRequest();
				request->setLocation(requests[i]->getLocation().first, requests[i]->getLocation().second);
				request->setRequestTime(requests[i]->getRequestTime());
				request->setDestination(requests[i]->getDestination().first, requests[i]->getDestination().second);
				if (requests[i]->getDestination().second < 0 || requests[i]->getDestination().first < 0){
					int x = 5;
				}
				managers[currentTest]->addRequest(request);
			}
		}
	}

	inline void initialize(bool getParameters){
		gridTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/grid_texture.png"), 0);
		requestTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/request_texture.png"), 1);
		//venueTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/venue_texture.png"), 1);
		destinationTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/destination_texture.png"), 1);
		lineTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/line_texture.png"), 1);
		vehicleTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/vehicle_texture.png"), 2);
		textSheet = new Texture(loadTexture(RESOURCE_FOLDER"Assets/text_format.png"), 2);
		offsetX = 0;
		offsetY = 0;
		scaleOffsetX = 0;
		scaleOffsetY = 0;
		windowSizeOffsetX = 0;
		windowSizeOffsetY = 0;
		if (!getParameters){
			DIR *dirp;
			struct dirent *dp;
			char fileName[360];
			char fileDirec[360] = RESOURCE_FOLDER"XML/";
			if ((dirp = opendir(fileDirec)) == NULL){
				throw "Could not find current directory!";
			}
			while ((dp = readdir(dirp)) != NULL){
				strcpy_s(fileName, dp->d_name);
				if (fileName[0] != '.' && fileName[1] != '.'){
					strcpy_s(fileDirec, RESOURCE_FOLDER"XML/");
					strcat_s(fileDirec, fileName);
					currentTest = fileName;
					currentTest = currentTest.substr(0, currentTest.find_first_of('.'));
					tests.push_back(currentTest);
					weightOfDistanceOfTrip[currentTest] = 0.002;
					timesToRun[currentTest] = 10;
					radiusMin[currentTest] = 5;
					radiusStep[currentTest] = 5;
					radiusMax[currentTest] = 15;
					timeRadius[currentTest] = 5;
					minimumScore[currentTest] = 5;
					maxRideRequests[currentTest] = 30;
					int fleetSize = 0, requestCount = 0, venueCount = 0;
					xml_document<>* doc = loadXMLFile(fileDirec);
					if (doc->first_node("Parameters") == nullptr){
						throw "Empty file!";
					}
					xml_node<>* parameters = doc->first_node("Parameters");
					if (parameters->first_attribute("WeightOfDistanceOfTrip") != nullptr){
						weightOfDistanceOfTrip[currentTest] = std::stof(parameters->first_attribute("WeightOfDistanceOfTrip")->value());
					}
					if (parameters->first_attribute("TimesToRun") != nullptr){
						timesToRun[currentTest] = std::stoi(parameters->first_attribute("TimesToRun")->value());
					}
					if (parameters->first_attribute("MinRadius") != nullptr){
						radiusMin[currentTest] = std::stoi(parameters->first_attribute("MinRadius")->value());
					}
					if (parameters->first_attribute("RadiusStep") != nullptr){
						radiusMin[currentTest] = std::stoi(parameters->first_attribute("RadiusStep")->value());
					}
					if (parameters->first_attribute("MaxRadius") != nullptr){
						radiusMin[currentTest] = std::stoi(parameters->first_attribute("MaxRadius")->value());
					}
					if (parameters->first_attribute("TimeRadius") != nullptr){
						timeRadius[currentTest] = std::stoi(parameters->first_attribute("TimeRadius")->value());
					}
					if (parameters->first_attribute("MinimumScore") != nullptr){
						minimumScore[currentTest] = std::stof(parameters->first_attribute("MinimumScore")->value());
					}
					if (parameters->first_attribute("MaxRideRequests") != nullptr){
						maxRideRequests[currentTest] = std::stoi(parameters->first_attribute("MaxRideRequests")->value());
					}

					if (parameters->first_node("RequestManager") == nullptr){
						throw "No request manager!";
					}
					enrichManagerData(parameters->first_node("RequestManager"));
					if (parameters->first_node("Requests") == nullptr){
						throw "No requests!";
					}
					if (parameters->first_node("Requests")->first_attribute("requestCount") != nullptr){
						requestCount = std::stoi(parameters->first_node("Requests")->first_attribute("requestCount")->value());
					}
					xml_node<>* requestNode = parameters->first_node("Requests")->first_node("Request");
					do{
						enrichRequestData(requestNode);
						requestNode = requestNode->next_sibling("Request");
						if (requestCount > 0){
							requestCount--;
						}
					} while (requestNode != nullptr);

					while (requestCount > 0){
						createRandomRequest(managers[currentTest]->getMinCoords().first, managers[currentTest]->getMinCoords().second, managers[currentTest]->getMaxCoords().first, managers[currentTest]->getMaxCoords().second);
						requestCount--;
					}

					if (parameters->first_node("Vehicles") == nullptr){
						throw "No vehicles!";
					}
					if (parameters->first_node("Vehicles")->first_attribute("fleetSize") != nullptr){
						fleetSize = std::stoi(parameters->first_node("Vehicles")->first_attribute("fleetSize")->value());
					}
					xml_node<>* vehicleNode = parameters->first_node("Vehicles")->first_node("Vehicle");
					do{
						enrichVehicleData(vehicleNode);
						vehicleNode = vehicleNode->next_sibling("Vehicle");
						if (fleetSize > 0){
							fleetSize--;
						}
					} while (vehicleNode != nullptr);

					while (fleetSize > 0){
						createRandomVehicle(managers[currentTest]->getMinCoords().first, managers[currentTest]->getMinCoords().second, managers[currentTest]->getMaxCoords().first, managers[currentTest]->getMaxCoords().second);
						fleetSize--;
					}

					/*if (parameters->first_node("Venues") != nullptr){
						if (parameters->first_node("Venues")->first_attribute("venueCount") != nullptr){
						venueCount = std::stoi(parameters->first_node("Venues")->first_attribute("venueCount")->value());
						}
						xml_node<>* venueNode = parameters->first_node("Venues")->first_node("Venue");
						do{
						enrichVenueData(venueNode);
						venueNode = venueNode->next_sibling("Venue");
						if (venueCount > 0){
						venueCount--;
						}
						} while (venueNode != nullptr);
						while (venueCount > 0){
						createRandomVenue(managers[currentTest]->getMinCoords().first, managers[currentTest]->getMinCoords().second, managers[currentTest]->getMaxCoords().first, managers[currentTest]->getMaxCoords().second);
						venueCount--;
						}
						}*/
					//outputTestData();
				}
			}
		}
	}

	inline void update(float fixedTimestep, const std::string& testName){
		for (Vehicle* vehicle : vehicles[testName]){
			vehicle->updateForRendering(fixedTimestep);
		}
	}

	inline void render(ShaderProgram* program, float elapsed, float framesPerSecond, int scaleX, int scaleY, const std::string& testName){
		managers[testName]->render(program, elapsed, timeRadius[testName], scaleX, scaleY);
		Matrix modelMatrix;
		std::vector<GLfloat> objectVertices;
		std::vector<GLfloat> textureCoordinates;
		std::vector<GLfloat> colorVector;
		for (int i = 0; i < 6; i++){
			colorVector.insert(colorVector.end(), { 1.0, 1.0, 1.0, 1.0 });
		}
		for (Vehicle* vehicle : vehicles[testName]){
			if (vehicle->checkRoutingLog()){
				modelMatrix.identity();
				modelMatrix.Translate(vehicle->getNextRoutingNode().second.second, vehicle->getNextRoutingNode().second.first + 0.5, 0);
				modelMatrix.Scale(scaleX, scaleY * 2, 0);
				objectVertices = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
				// {-x, -y}, {x, -y}, {x, y}
				// {-x, -y}, {x, y}, {-x, y}

				textureCoordinates = destinationTexture->getTextureCoordinates();
				glBindTexture(GL_TEXTURE_2D, destinationTexture->getTextureID());
				program->setModelMatrix(modelMatrix);

				glEnableVertexAttribArray(program->positionAttribute);
				glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, objectVertices.data());

				glEnableVertexAttribArray(program->texCoordAttribute);
				glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordinates.data());

				glDrawArrays(GL_TRIANGLES, 0, 6);
				glDisableVertexAttribArray(program->positionAttribute);
				glDisableVertexAttribArray(program->texCoordAttribute);

				float lineWidth = 0.5;

				float destX = vehicle->getNextRoutingNode().second.second;
				float destY = vehicle->getNextRoutingNode().second.first;

				float locX = vehicle->getPreviousRenderingLocation().second;
				float locY = vehicle->getPreviousRenderingLocation().first;

				float dist = pythagDistance(locX, locY, destX, destY);

				// Triangle Right: {-x, -y}, {x, -y}, {x, y}
				// Triangle Left: {-x, -y}, {x, y}, {-x, y}
				float centerX = (locX + destX) / 2;
				float centerY = (locY + destY) / 2;

				//float angle = atan2f(destY - locY, destX - locX);
				float angle = abs(acosf((destY - locY) / dist));
				while (angle >= M_PI){
					angle -= M_PI;
				}
				//if (angle > 3.14159 / 4 && angle < 3.14159 / 3){
				//	angle *= -1;
				//}
				if (destX > locX){
					angle *= -1;
				}
				textureCoordinates = lineTexture->getTextureCoordinates();
				glBindTexture(GL_TEXTURE_2D, lineTexture->getTextureID());
				modelMatrix.identity();
				modelMatrix.Translate(centerX, centerY, 0);
				modelMatrix.Rotate(angle);
				modelMatrix.Scale(lineWidth, dist, 0);
				program->setModelMatrix(modelMatrix);

				glEnableVertexAttribArray(program->positionAttribute);
				glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, objectVertices.data());

				glEnableVertexAttribArray(program->texCoordAttribute);
				glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordinates.data());

				glDrawArrays(GL_TRIANGLES, 0, 6);
				glDisableVertexAttribArray(program->positionAttribute);
				glDisableVertexAttribArray(program->texCoordAttribute);
			}
		}
		for (Vehicle* vehicle : vehicles[testName]){
			modelMatrix.identity();
			modelMatrix.Translate(vehicle->getCurrentRenderingLocation().second, vehicle->getCurrentRenderingLocation().first, 0);
			modelMatrix.Scale(scaleX + 0.5, scaleY + 0.5, 0);
			modelMatrix.Rotate(vehicle->getRenderingAngle());
			objectVertices = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
			//-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1
			//n, n, p, n, p, p, n, n, p, p, n, p

			textureCoordinates = vehicleTexture->getTextureCoordinates();
			glBindTexture(GL_TEXTURE_2D, vehicleTexture->getTextureID());
			program->setModelMatrix(modelMatrix);

			glEnableVertexAttribArray(program->positionAttribute);
			glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, objectVertices.data());

			glEnableVertexAttribArray(program->texCoordAttribute);
			glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordinates.data());

			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(program->positionAttribute);
			glDisableVertexAttribArray(program->texCoordAttribute);
		}
		float texture_size = 1.0 / 16.0f;
		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		std::vector<float> color;
		float timeToUse = roundf(elapsed * 1000.0) / 1000.0;
		std::stringstream textStream;
		//text = "Time: %f" % timeToUse;
		//text += std::to_string(timeToUse);
		textStream.precision(3);
		textStream << "Time: " << timeToUse;
		std::string text = textStream.str();
		modelMatrix.identity();
		modelMatrix.Translate(0, -3, 0);
		float textScaleX = (getMaxCoords(testName).second / getSectionRadius(testName)) * 0.375;
		float textScaleY = (getMaxCoords(testName).first / getSectionRadius(testName)) * 0.375;
		modelMatrix.Scale(textScaleX, textScaleY, 0);
		float size = 0.5;
		float spacing = 0.2;
		for (int i = 0; i < text.size(); i++) {
			float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
			float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
			vertexData.insert(vertexData.end(), { ((size + spacing) * i) + (-0.5f * size), 0.5f * size, ((size + spacing) * i) + (-0.5f * size), -0.5f * size, ((size + spacing) * i) + (0.5f * size), 0.5f * size, ((size + spacing) * i) + (0.5f * size), -0.5f * size, ((size + spacing) * i) + (0.5f * size), 0.5f * size, ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
			texCoordData.insert(texCoordData.end(), { texture_x, texture_y,
				texture_x, texture_y + texture_size, texture_x + texture_size, texture_y, texture_x + texture_size, texture_y + texture_size, texture_x + texture_size, texture_y, texture_x, texture_y + texture_size,
			});
		}
		glUseProgram(program->programID);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		program->setModelMatrix(modelMatrix);


		glBindTexture(GL_TEXTURE_2D, textSheet->getTextureID());
		glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	inline bool runTest(int testNum){
		std::cout << "Test " << std::to_string(testNum + 1) << " of " + std::to_string(tests.size()) << " started..." << std::endl;
		int currentTime = 1;
		std::string testName = tests[testNum];
		std::ofstream outputFile;
		if (!runningRanged){
			char resultsFile[360] = RESOURCE_FOLDER"Results/";
			strcat_s(resultsFile, testName.c_str());
			strcat_s(resultsFile, ".txt");
			outputFile.open(resultsFile);
			outputFile << testName << " Data:" << std::endl << std::endl;

			outputFile << "Testing Parameters:" << std::endl << std::endl;
			outputFile << "\tTimes to run: " << std::to_string(timesToRun[testName]) << std::endl << std::endl;

			outputFile << "\tMinimum Search Radius: " << std::to_string(radiusMin[testName]) << std::endl;
			outputFile << "\tSearch Radius Step: " << std::to_string(radiusStep[testName]) << std::endl;
			outputFile << "\tMaximum Search Radius: " << std::to_string(radiusMax[testName]) << std::endl << std::endl;

			outputFile << "\tTime Radius: " << std::to_string(timeRadius[testName]) << std::endl << std::endl;

			outputFile << "\tRide Request Score Calculations: " << std::endl << std::endl;
			outputFile << "\t\tDistance Calculated by: Pythagorean Theorem" << std::endl;
			outputFile << "\t\tTime To Distance Ratio: 1:1" << std::endl << std::endl;

			outputFile << "\t\tPickup_Distance = Distance from car to pickup point (could also use time to arrive at pickup)" << std::endl;
			outputFile << "\t\tRide_Distance = Distance from pickup to the car's destination (could also use time to arrive at destination)" << std::endl;
			outputFile << "\t\tRequests_At_Destination = Requests projected to exist at the destination point + Requests scheduled near destination at arrival time" << std::endl << std::endl;

			outputFile << "\t\tRequests_At_Destination_Ceiling = Minimum number of requests available at destination to result in no penalty to score = " << std::to_string(maxRideRequests[testName]) << std::endl;
			outputFile << "\t\tRide_Distance_Weight = Coeficient used to determine relative value of the distance of the trip = " << std::to_string(weightOfDistanceOfTrip[testName]) << std::endl << std::endl;

			outputFile << "\t\tPercentageValueOfTrip = ((Ride_Distance)/(Ride_Distance + Pickup_Distance)) * 10" << std::endl;
			outputFile << "\t\tValueOfTripDistance = (Ride_Distance * Ride_Distance_Weight) * 10" << std::endl;
			outputFile << "\t\tDestinationPenalty:" << std::endl;
			outputFile << "\t\t\tIf Requests_At_Destination > Requests_At_Destination_Saturation, DestinationPenalty = 0" << std::endl;
			outputFile << "\t\t\tOtherwise, DestinationPenalty = -3 + (Requests_At_Destination * (3/Requests_At_Destination_Ceiling))" << std::endl << std::endl;

			outputFile << "\t\tRequest_Score = PercentageValueOfTrip + ValueOfTripLength + DestinationPenalty" << std::endl << std::endl;
		}
		else{
			numberOfCompletedRequests[testNum] = 0;
		}
		for (int i = 1; i <= timesToRun[testName]; i++){
			int vehicleNum = 1;
			for (Vehicle* vehicle : vehicles[testName]){
				float topScore = 0;
				vehicle->update(currentTime, timeRadius[testName]);
				std::pair<long, long> vehicleLocation = vehicle->getCurrentLocation();
				std::pair<long, long> radiusLookUp, radiusLookLeft, radiusLookRight, radiusLookDown;

				if (vehicle->getTopRequest() == nullptr || (vehicle->getTopRequest() != nullptr && (vehicleLocation.first == vehicle->getTopRequest()->getDestination().first && vehicleLocation.second == vehicle->getTopRequest()->getDestination().second))){
					if (vehicle->getTopRequest() != nullptr && (vehicleLocation.first == vehicle->getTopRequest()->getDestination().first && vehicleLocation.second == vehicle->getTopRequest()->getDestination().second)){
						vehicle->setHasPassenger(false);
						vehicle->popTopRequest();
						vehicle->addToRoutingLog(i, vehicleLocation);
						numberOfCompletedRequests[testNum]++;
					}
					for (int x = radiusMin[testName]; x <= radiusMax[testName]; x += radiusStep[testName]){
						RideRequest* highestScorer = nullptr;
						radiusLookUp = radiusLookLeft = radiusLookRight = radiusLookDown = vehicleLocation;
						radiusLookUp.first += x;
						radiusLookLeft.second -= x;
						radiusLookRight.second += x;
						radiusLookDown.first -= x;
						std::vector<RideRequest*>* requests = &managers[testName]->getRequestsAtLocation(vehicleLocation);
						std::vector<RideRequest*>* upRequests = &managers[testName]->getRequestsAtLocation(radiusLookUp);
						std::vector<RideRequest*>* downRequests = &managers[testName]->getRequestsAtLocation(radiusLookDown);
						std::vector<RideRequest*>* leftRequests = &managers[testName]->getRequestsAtLocation(radiusLookLeft);
						std::vector<RideRequest*>* rightRequests = &managers[testName]->getRequestsAtLocation(radiusLookRight);
						topScore = minimumScore[testName];
						if (requests->size() != 0){
							for (RideRequest* request : *requests){
								if (!(request)->getMatchedToVehicle() && scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance) > topScore){
									topScore = scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance);
									highestScorer = request;
								}
							}
						}
						if (upRequests->size() != 0 && (upRequests != requests)){
							for (RideRequest* request : *upRequests){
								if (!(request)->getMatchedToVehicle() && scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance) > topScore){
									topScore = scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance);
									highestScorer = request;
									requests = upRequests;
								}
							}
						}
						if (downRequests->size() != 0 && (downRequests != requests || (downRequests == requests && topScore == 0))){
							for (RideRequest* request : *downRequests){
								if (!(request)->getMatchedToVehicle() && scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance) > topScore){
									topScore = scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance);
									highestScorer = request;
									requests = downRequests;
								}
							}
						}
						if (leftRequests->size() != 0 && (leftRequests != requests || (leftRequests == requests && topScore == 0))){
							for (RideRequest* request : *leftRequests){
								if (!(request)->getMatchedToVehicle() && scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance) > topScore){
									topScore = scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance);
									highestScorer = request;
									requests = leftRequests;
								}
							}
						}
						if (rightRequests->size() != 0 && (rightRequests != requests || (rightRequests == requests && topScore == 0))){
							for (RideRequest* request : *rightRequests){
								if (!(request)->getMatchedToVehicle() && scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance) > topScore){
									topScore = scoreRequest(vehicle, request, managers[testName], currentTime, timeRadius[testName], weightOfDistanceOfTrip[testName], maxRideRequests[testName], &pythagDistance);
									highestScorer = request;
									requests = rightRequests;
								}
							}
						}
						if (topScore > minimumScore[testName] && highestScorer != nullptr){
							int timeToUse = currentTime;
							if (vehicle->getTopRequest() != nullptr){
								timeToUse = vehicle->getTopRequest()->getRequestTime() + vehicle->getTopRequest()->getDistanceOfRequest();
							}
							vehicle->addRequest(highestScorer);
							(highestScorer)->setMatchedToVehicle(true);
							(highestScorer)->setTimeMatched(i);
							break;
						}
					}
				}

				//handle output after scanning
				try{
					if (vehicle->getTopRequest() != nullptr && vehicle->getTopRequest()->getPickedUp()){
						if (!vehicle->getHasPassenger()){
							//outputFile << "\t\tVehicle picked up request at Latitude: " << vehicleLocation.first << " and Longitude: " << vehicleLocation.second << " at T = " << currentTime << '\n';
							vehicle->setHasPassenger(true);
							vehicle->addToRoutingLog(i, vehicleLocation);
						}
						else{
							vehicle->setHasPassenger(true);
							//outputFile << "\t\tVehicle en route to destination." << '\n';
						}
					}
					else if (vehicle->getTopRequest() != nullptr && !vehicle->getTopRequest()->getPickedUp()){
						if (vehicle->getCurrentLocation().first != vehicle->getTopRequest()->getLocation().first && vehicle->getCurrentLocation().second != vehicle->getTopRequest()->getLocation().second){
							//outputFile << "\t\tVehicle en route to pickup." << '\n';
							vehicle->setHasPassenger(false);
						}
						else{
							vehicle->setHasPassenger(false);
							//outputFile << "\t\tVehicle waiting at pickup for request." << '\n';
						}
					}
					else if (vehicle->getTopRequest() == nullptr){
						//outputFile << "\t\tVehicle idling.";
						vehicle->setHasPassenger(false);
					}
				}
				catch (std::exception & e){
					vehicle->popTopRequest();
					//outputFile << "\t\tVehicle status unknown.";
				}

				vehicleNum++;
				//outputFile << '\n' << '\n';
			}
			currentTime++;
			//outputFile << '\n';
		}

		if (!runningRanged){
			outputFile << '\n';
			outputFile << "-------------------------------------------------------------------" << '\n';
			outputFile << "Complete Test Results:" << '\n' << '\n';
		}
		int vehicleNum = 1;
		float distanceWithPassenger = 0;
		float distanceWithoutPassenger = 0;
		float percentUtilization = 100;
		for (Vehicle* vehicle : vehicles[testName]){
			if (!runningRanged){
				outputFile << "Vehicle: " << vehicleNum << '\n';
				outputFile << '\t' << "Distance travelled with a passenger: " << vehicle->getDistanceWithPassenger() << '\n';
				outputFile << '\t' << "Distance travelled without a passenger: " << vehicle->getDistanceWithoutPassenger() << '\n';
				outputFile << '\n' << '\n';
			}
			distanceWithPassenger += vehicle->getDistanceWithPassenger();
			distanceWithoutPassenger += vehicle->getDistanceWithoutPassenger();
			vehicleNum++;
		}
		if (distanceWithPassenger + distanceWithoutPassenger != 0){
			percentUtilization = (distanceWithPassenger) / (distanceWithoutPassenger + distanceWithPassenger);
		}
		else{
			percentUtilization = 0;
		}
		//percentUtilization *= 100;
		if (!runningRanged){
			outputFile << "Fleet utilization is: " << percentUtilization << "%.";
			outputFile << "\nTotal distance travelled with passengers: " << distanceWithoutPassenger << ".";
			outputFile.close();
		}
		//std::cout << "\n\n";
		if (runningRanged){
			results[testNum] = percentUtilization;
			totalDistanceWithPassenger[testNum] = distanceWithPassenger;
			totalDistanceWithoutPassenger[testNum] = distanceWithoutPassenger;
		}
		std::cout << "Test " << std::to_string(testNum + 1) << " of " << std::to_string(tests.size()) << " Completed." << std::endl;
		return true;
	}

	inline void prepareResultsMatrix(BasicExcelWorksheet* worksheet){
		BasicExcelCell* cell;
		size_t row = 0;
		size_t column = 0;
		std::vector<const char*> columnNames = { "Trip Distance Weight", "Minimum Search Radius", "Maximum Search Radius", "Time Radius", "Minimum Score", "Maximum Destination Requests", "Distance Travelled with Passenger", "Distance Travelled Without Passenger", "Requests Completed", "Percent Utilization" };
		for (; column < columnNames.size(); column++){
			cell = worksheet->Cell(row, column);
			cell->SetString(columnNames[column]);
		}
	}

	inline void outputToExcelFile(){
		BasicExcel outputFile;
		std::string saveLocation = RESOURCE_FOLDER"Aggregate Results/" + excelFileName;
		outputFile.New(1);
		outputFile.RenameWorksheet("Sheet1", "Results Matrix");
		if (outputFile.worksheets_.empty()){
			outputFile.SaveAs(saveLocation.c_str());
		}
		BasicExcelWorksheet* resultsWorksheet = outputFile.GetWorksheet("Results Matrix");
		prepareResultsMatrix(resultsWorksheet);
		int row = 1;
		int column = 0;
		BasicExcelCell* cell;
		for (size_t i = 0; i < tests.size(); i++){
			std::string testName = tests[i];
			column = 0;
			cell = resultsWorksheet->Cell(row, column++);
			cell->SetDouble(weightOfDistanceOfTrip[testName]);

			cell = resultsWorksheet->Cell(row, column++);
			cell->SetInteger(radiusMin[testName]);

			cell = resultsWorksheet->Cell(row, column++);
			cell->SetInteger(radiusMax[testName]);

			cell = resultsWorksheet->Cell(row, column++);
			cell->SetInteger(timeRadius[testName]);

			cell = resultsWorksheet->Cell(row, column++);
			cell->SetDouble(minimumScore[testName]);

			cell = resultsWorksheet->Cell(row, column++);
			cell->SetInteger(maxRideRequests[testName]);

			cell = resultsWorksheet->Cell(row, column++);
			cell->SetInteger(totalDistanceWithPassenger[i]);

			cell = resultsWorksheet->Cell(row, column++);
			cell->SetInteger(totalDistanceWithoutPassenger[i]);

			cell = resultsWorksheet->Cell(row, column++);
			cell->SetInteger(numberOfCompletedRequests[i]);
			
			cell = resultsWorksheet->Cell(row++, column);
			cell->SetDouble(results[i]);
		}

		outputFile.SaveAs(saveLocation.c_str());
	}

public:
	Simulator(bool getParameters = false){
		initialize(getParameters);
	}

	void initializeSimulatorWithParams(const std::string& customTestName, unsigned customTimesToRun, float customTripWeight, float customRadiusMin, float customRadiusStep, float customRadiusMax,
		float customTimeRadius, float customMinimumScore, unsigned customMaximumRideRequests, unsigned customFleetSize,
		unsigned customRideCount, float customMaxLat, float customMaxLong, float customSectionSize, bool rangedTest = false){
		initialize(customTestName, customTimesToRun, customTripWeight, customRadiusMin, customRadiusStep, customRadiusMax, customTimeRadius, customMinimumScore,
			customMaximumRideRequests, customFleetSize, customRideCount, customMaxLat, customMaxLong, customSectionSize, rangedTest);
	}

	inline void runTests(){
		std::queue<std::future<bool>> testThreads;
		for (size_t testNum = 0; testNum < tests.size(); testNum++){
			std::future<bool> test = std::async(&Simulator::runTest, this, testNum);
			testThreads.push(std::move(test));
		}
		std::cout << std::endl << std::endl;
		while (testThreads.size() != 0){
			if (testThreads.front().wait_for(std::chrono::seconds(0)) == std::future_status::ready){
				testThreads.pop();
			}
		}
		if (runningRanged){
			outputToExcelFile();
		}
	}
	inline const std::vector<std::string>& getTestNames(){
		return tests;
	}

	inline int getTimesToRun(const std::string& testName){
		return timesToRun[testName];
	}

	inline void prepareToRender(){
		for (int i = 0; i < tests.size(); i++){
			for (Vehicle* vehicle : vehicles[tests[i]]){
				vehicle->prepareForRendering();
			}
		}
	}

	inline void visualize(float elapsed, const Uint8* input, SDL_Event input2, ShaderProgram* program, const std::string& testName, SDL_Window* displayWindow){
		glClear(GL_COLOR_BUFFER_BIT);
		Matrix viewMatrix;
		//viewMatrix.Scale((float)managers[testName]->getSectionRadius() / (float)managers[testName]->getMaxCoords().second, ((float)managers[testName]->getSectionRadius() / (float)managers[testName]->getMaxCoords().first)/2, 0);
		//viewMatrix.Translate(-(managers[testName]->getSectionRadius() * 2)-((managers[testName]->getMaxCoords().first) / managers[testName]->getMaxCoords().second), -(managers[testName]->getSectionRadius() * 2) -(managers[testName]->getMaxCoords().second / managers[testName]->getMaxCoords().first), 0);
		int scaleXCoeff = (managers[testName]->getMaxCoords().first / managers[testName]->getSectionRadius()) % managers[testName]->getSectionRadius();
		if (scaleXCoeff == 0 || scaleXCoeff == 4){
			scaleXCoeff = 1;
		}
		int scaleYCoeff = (managers[testName]->getMaxCoords().second / managers[testName]->getSectionRadius()) % managers[testName]->getSectionRadius();
		if (scaleYCoeff == 0 || scaleYCoeff == 4){
			scaleYCoeff = 1;
		}
		float scaleX = (float)managers[testName]->getSectionRadius() / (float)managers[testName]->getMaxCoords().second * scaleXCoeff;
		float scaleY = (float)managers[testName]->getSectionRadius() / (float)managers[testName]->getMaxCoords().first * scaleYCoeff;
		float translateX = -(((float)managers[testName]->getMaxCoords().second / (float)managers[testName]->getSectionRadius()) * 2) - ((managers[testName]->getMaxCoords().second / managers[testName]->getMaxCoords().first) * 2);
		float translateY = -(((float)managers[testName]->getMaxCoords().first / (float)managers[testName]->getSectionRadius()) * 2) - ((managers[testName]->getMaxCoords().second / managers[testName]->getMaxCoords().first) * 2);
		windowSizeOffsetX = getMaxCoords(testName).second;
		windowSizeOffsetY = getMaxCoords(testName).first;
		viewMatrix.Scale(scaleX + scaleOffsetX, scaleY + scaleOffsetY, 0);
		viewMatrix.Translate(translateX + offsetX, translateY + offsetY, 0);
		SDL_SetWindowSize(displayWindow, 720, 800);
		glViewport(0, 0, 720, 800);
		Matrix projectionMatrix;
		int coeffX = (int)(getMaxCoords(testName).second / getSectionRadius(testName));
		int coeffY = (int)(getMaxCoords(testName).first / getSectionRadius(testName));
		if (coeffX < 10){
			coeffX *= 10;
			offsetX = 0;
		}
		else{
			offsetX = -(getMaxCoords(testName).second / getSectionRadius(testName) + getSectionRadius(testName));
		}

		if (coeffY < 10){
			coeffY *= 10;
			offsetY = 0;
		}
		else{
			offsetY = -(getMaxCoords(testName).first / getSectionRadius(testName) + getSectionRadius(testName));
		}
		projectionMatrix.setOrthoProjection(-(640 + windowSizeOffsetX * coeffX) / 360, (640 + windowSizeOffsetX * coeffX) / 360, -(640 + windowSizeOffsetY * coeffY) / 360, (640 + windowSizeOffsetY * coeffY) / 360, -1.0, 1.0);
		program->setProjectionMatrix(projectionMatrix);
		program->setViewMatrix(viewMatrix);
		update(elapsed, testName);
		float elementScaleX = ((float)(getMaxCoords(testName).second) / (float)(getSectionRadius(testName))) / 4;
		float elementScaleY = ((float)(getMaxCoords(testName).first) / (float)(getSectionRadius(testName))) / 4;
		render(program, elapsed, FRAMES_PER_SECOND, elementScaleX, elementScaleY, testName);
	}

	inline void freeMemory(){
		delete lineTexture;
		delete vehicleTexture;
		delete requestTexture;
		delete destinationTexture;
		delete textSheet;
		lineTexture = nullptr;
		vehicleTexture = nullptr;
		requestTexture = nullptr;
		destinationTexture = nullptr;
		gridTexture = nullptr;
		textSheet = nullptr;
		for (std::unordered_map<std::string, RequestManager*>::iterator itr = managers.begin(); itr != managers.end(); itr++){
			itr->second->freeMemory();
			delete itr->second;
			itr->second = nullptr;
		}
		managers.clear();

		for (std::unordered_map<std::string, std::vector<Vehicle*>>::iterator itr = vehicles.begin(); itr != vehicles.end(); itr++){
			for (Vehicle* vehicle : itr->second){
				vehicle->freeMemory();
				delete vehicle;
				vehicle = nullptr;
			}
			itr->second.clear();
		}
		vehicles.clear();
	}

	inline std::pair<int, int> getMaxCoords(const std::string& testName){
		return managers[testName]->getMaxCoords();
	}

	inline int getSectionRadius(const std::string& testName){
		return managers[testName]->getSectionRadius();
	}

	inline void setExcelFileName(const std::string& excelFileName){
		this->excelFileName = excelFileName;
	}

};

#endif