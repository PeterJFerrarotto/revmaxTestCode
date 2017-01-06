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
#include "EventVenue.h"
#include <vector>
#include <iostream>
#include "renderingMathHelper.h"
#include "Texture.h"

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
	std::unordered_map<std::string, RequestManager> managers;
	std::vector<std::string> tests;
	std::string currentTest;

	std::unordered_map<std::string, float> weightOfDistanceOfTrip;
	std::unordered_map<std::string, unsigned> timesToRun;
	std::unordered_map<std::string, float> radiusMin;
	std::unordered_map<std::string, float> radiusStep;
	std::unordered_map<std::string, float> radiusMax;
	std::unordered_map<std::string, float> timeRadius;
	std::unordered_map<std::string, float> minimumScore;
	std::unordered_map<std::string, int> maxRideRequests;

	Texture* lineTexture;
	Texture* requestTexture;
	Texture* venueTexture;
	Texture* vehicleTexture;
	float simTime;
	inline xml_document<> * loadXMLFile(const char* filePath){
		file<>* xmlFile = new file<>(filePath);
		xml_document<>* doc = new xml_document<>;
		doc->parse<0>(xmlFile->data());
		return doc;
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
		managers[currentTest].addRequest(request);
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
		managers[currentTest].addRequest(request);
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
		vehicle->setLocation(lat, longitude);
		vehicles[currentTest].push_back(vehicle);
	}

	inline void createRandomVehicle(float minLat, float minLong, float maxLat, float maxLong){
		float locLat, locLong;
		locLat = randomRangedLong(minLat, maxLat);
		locLong = randomRangedLong(minLong, maxLong);
		Vehicle* vehicle = new Vehicle;
		vehicle->setLocation(locLat, locLong);
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
		
		managers[currentTest].setLatitudeMin(minLat);
		managers[currentTest].setLongitudeMin(minLong);
		managers[currentTest].setLatitudeMax(maxLat);
		managers[currentTest].setLongitudeMax(maxLong);
		managers[currentTest].setSectionRadius(sectionSize);
		managers[currentTest].initializeRequestMap();

		managers[currentTest].setLineTexture(lineTexture);
		managers[currentTest].setRequestTexture(requestTexture);
		managers[currentTest].setVenueTexture(venueTexture);
	}

	inline void enrichVenueData(xml_node<>* venueNode){
		float latitude = 0, longitude = 0;
		EventVenue* venue = new EventVenue();
		if (venueNode->first_node("Location") == nullptr){
			throw "No location specified for venue!";
		}

		xml_node<>* locationNode = venueNode->first_node("Location");
		if (locationNode->first_attribute("long") == nullptr || locationNode->first_attribute("lat") == nullptr){
			throw "No latitude or longitude!";
		}

		latitude = std::stof(locationNode->first_attribute("lat")->value());
		longitude = std::stof(locationNode->first_attribute("long")->value());

		if (venueNode->first_node("Events") != nullptr){
			xml_node<>* requestNode = venueNode->first_node("Events")->first_node("Event");
			if (requestNode != nullptr){
				do{
					int time, requestCount;
					if (requestNode->first_attribute("time") == nullptr || requestNode->first_attribute("requestCount") == nullptr){
						throw "No time or request count specified for event!";
					}
					time = std::stoi(requestNode->first_attribute("time")->value());
					requestCount = std::stoi(requestNode->first_attribute("requestCount")->value());
					venue->addEvent(time, requestCount);
					requestNode = requestNode->next_sibling("Event");
				} while (requestNode != nullptr);
			}
		}
		venue->setLocation(latitude, longitude);
		managers[currentTest].addVenue(venue);
	}

	inline void createRandomVenue(float minLat, float minLong, float maxLat, float maxLong){
		float latitude, longitude;
		int eventCount, eventTime, eventRequestCount;
		latitude = randomRangedLong(minLat, maxLat);
		longitude = randomRangedLong(minLong, maxLong);
		EventVenue* venue = new EventVenue();
		venue->setLocation(latitude, longitude);
		eventCount = randomRangedInt(0, 10);
		for (int i = 0; i < eventCount; i++){
			eventTime = randomRangedInt(0, timesToRun[currentTest]);
			eventRequestCount = randomRangedInt(0, maxRideRequests[currentTest]);
			venue->addEvent(eventTime, eventRequestCount);
		}
		managers[currentTest].addVenue(venue);
	}

	inline void initialize(bool getParameters){
		lineTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/line_texture.png"), 0);
		requestTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/request_texture.png"), 1);
		venueTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/venue_texture.png"), 1);
		vehicleTexture = new Texture(loadTexture(RESOURCE_FOLDER"Assets/vehicle_texture.png"), 2);
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
					strcpy_s(fileDirec, "XML/");
					strcat_s(fileDirec, fileName);
					currentTest = fileName;
					currentTest = currentTest.substr(0, currentTest.find_first_of('.'));
					tests.push_back(currentTest);
					weightOfDistanceOfTrip[currentTest] = 20;
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
						createRandomRequest(managers[currentTest].getMinCoords().first, managers[currentTest].getMinCoords().second, managers[currentTest].getMaxCoords().first, managers[currentTest].getMaxCoords().second);
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
						createRandomVehicle(managers[currentTest].getMinCoords().first, managers[currentTest].getMinCoords().second, managers[currentTest].getMaxCoords().first, managers[currentTest].getMaxCoords().second);
						fleetSize--;
					}

					if (parameters->first_node("Venues") != nullptr){
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
							createRandomVenue(managers[currentTest].getMinCoords().first, managers[currentTest].getMinCoords().second, managers[currentTest].getMaxCoords().first, managers[currentTest].getMaxCoords().second);
							venueCount--;
						}
					}
				}
			}
		}
	}

	inline void update(float fixedTimestep, const std::string& testName){
		for (Vehicle* vehicle : vehicles[testName]){
			vehicle->updateForRendering(fixedTimestep);
		}
	}


	inline void render(ShaderProgram* program, float elapsed, float framesPerSecond, const std::string& testName){
		managers[testName].render(program, elapsed, timeRadius[testName]);
		Matrix modelMatrix;
		std::vector<GLfloat> objectVertices;
		std::vector<GLfloat> textureCoordinates;
		for (Vehicle* vehicle : vehicles[testName]){
			modelMatrix.identity();
			modelMatrix.Translate(vehicle->getCurrentRenderingLocation().second, vehicle->getCurrentRenderingLocation().first, 0);
			modelMatrix.Scale(1, 1, 0);
			//modelMatrix.Rotate(vehicle->getRenderingAngle());
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
	}

public:
	Simulator(bool getParameters = false){
		initialize(getParameters);
	}

	inline void runTests(){
		for (size_t j = 0; j < tests.size(); j++){
			int currentTime = 1;
			currentTest = tests[j];
			char resultsFile[360] = RESOURCE_FOLDER"Results/";
			strcat_s(resultsFile, currentTest.c_str());
			strcat_s(resultsFile, ".txt");
			std::ofstream outputFile;
			outputFile.open(resultsFile);
			std::cout << "Test " << currentTest << ":" << '\n' << '\n' << '\n';
			outputFile << "Test " << currentTest << '\n';
			outputFile << "Parameters:" << '\n';
			outputFile << '\t' << "Weight of Distance of Request: " << weightOfDistanceOfTrip[currentTest] << '\n';
			outputFile << '\t' << "Minimum Search Radius: " << radiusMin[currentTest] << '\n';
			outputFile << '\t' << "Maximum Search Radius: " << radiusMax[currentTest] << '\n';
			outputFile << '\t' << "Radius Increment Value: " << radiusStep[currentTest] << '\n';
			outputFile << '\t' << "Time Radius: " << timeRadius[currentTest] << '\n';
			outputFile << '\t' << "Minimum Score: " << minimumScore[currentTest] << '\n';
			outputFile << '\t' << "Times to run: " << timesToRun[currentTest] << '\n' << '\n';
			for (int i = 1; i <= timesToRun[currentTest]; i++){
				std::cout << "Test loop " << i << " of " << timesToRun[currentTest] << '\n' << '\n';
				outputFile << "Test loop " << i << " of " << timesToRun[currentTest] << '\n' << '\n';
				int vehicleNum = 1;
				for (Vehicle* vehicle : vehicles[currentTest]){
					float topScore = 0;
					std::cout << "Vehicle: " << vehicleNum << '\n';
					outputFile << "\tVehicle: " << vehicleNum << '\n';
					vehicle->update(currentTime, timeRadius[currentTest]);
					std::pair<long, long> vehicleLocation = vehicle->getCurrentLocation();
					std::pair<long, long> radiusLookUp, radiusLookLeft, radiusLookRight, radiusLookDown;

					if(vehicle->getTopRequest() == nullptr || (vehicle->getTopRequest() != nullptr && (vehicleLocation.first == vehicle->getTopRequest()->getDestination().first && vehicleLocation.second == vehicle->getTopRequest()->getDestination().second))){
						if (vehicle->getTopRequest() != nullptr && (vehicleLocation.first == vehicle->getTopRequest()->getDestination().first && vehicleLocation.second == vehicle->getTopRequest()->getDestination().second)){
							vehicle->setHasPassenger(false);
							std::cout << "\t\tVehicle dropped off request at Latitude: " << vehicleLocation.first << " and Longitude: " << vehicleLocation.second << " at T = " << currentTime << '\n';
							outputFile << "\t\tVehicle dropped off request at Latitude: " << vehicleLocation.first << " and Longitude: " << vehicleLocation.second << " at T = " << currentTime << '\n';
							vehicle->popTopRequest();
							vehicle->addToRoutingLog(i, vehicleLocation);
						}
						for (int x = radiusMin[currentTest]; x <= radiusMax[currentTest]; x += radiusStep[currentTest]){
							std::vector<RideRequest*>::iterator highestScorer;
							radiusLookUp = radiusLookLeft = radiusLookRight = radiusLookDown = vehicleLocation;
							radiusLookUp.first += x;
							radiusLookLeft.second -= x;
							radiusLookRight.second += x;
							radiusLookDown.first -= x;
							std::vector<RideRequest*>* requests = &managers[currentTest].getRequestsAtLocation(vehicleLocation);
							std::vector<RideRequest*>* upRequests = &managers[currentTest].getRequestsAtLocation(radiusLookUp);
							std::vector<RideRequest*>* downRequests = &managers[currentTest].getRequestsAtLocation(radiusLookDown);
							std::vector<RideRequest*>* leftRequests = &managers[currentTest].getRequestsAtLocation(radiusLookLeft);
							std::vector<RideRequest*>* rightRequests = &managers[currentTest].getRequestsAtLocation(radiusLookRight);
							topScore = minimumScore[currentTest];
							if (requests->size() != 0){
								for (std::vector<RideRequest*>::iterator request = requests->begin(); request != requests->end(); request++){
									if (!(*request)->getMatchedToVehicle() && scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance) > topScore){
										topScore = scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance);
										highestScorer = request;
									}
								}
							}
							if (upRequests->size() != 0 && (upRequests != requests)){
								for (std::vector<RideRequest*>::iterator request = upRequests->begin(); request != upRequests->end(); request++){
									if (!(*request)->getMatchedToVehicle() && scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance) > topScore){
										topScore = scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance);
										highestScorer = request;
										requests = upRequests;
									}
								}
							}
							if (downRequests->size() != 0 && (downRequests != requests || (downRequests == requests && topScore == 0))){
								for (std::vector<RideRequest*>::iterator request = downRequests->begin(); request != downRequests->end(); request++){
									if (!(*request)->getMatchedToVehicle() && scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance) > topScore){
										topScore = scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance);
										highestScorer = request;
										requests = downRequests;
									}
								}
							}
							if (leftRequests->size() != 0 && (leftRequests != requests || (leftRequests == requests && topScore == 0))){
								for (std::vector<RideRequest*>::iterator request = leftRequests->begin(); request != leftRequests->end(); request++){
									if (!(*request)->getMatchedToVehicle() && scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance) > topScore){
										topScore = scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance);
										highestScorer = request;
										requests = leftRequests;
									}
								}
							}
							if (rightRequests->size() != 0 && (rightRequests != requests || ( rightRequests == requests && topScore == 0))){
								for (std::vector<RideRequest*>::iterator request = rightRequests->begin(); request != rightRequests->end(); request++){
									if (!(*request)->getMatchedToVehicle() && scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance) > topScore){
										topScore = scoreRequest(vehicle, *request, managers[currentTest], currentTime, timeRadius[currentTest], weightOfDistanceOfTrip[currentTest], maxRideRequests[currentTest], &pythagDistance);
										highestScorer = request;
										requests = rightRequests;
									}
								}
							}

							if (topScore > minimumScore[currentTest] && *highestScorer != nullptr){
								int timeToUse = currentTime;
								if (vehicle->getTopRequest() != nullptr){
									timeToUse = vehicle->getTopRequest()->getRequestTime() + vehicle->getTopRequest()->getDistanceOfRequest();
								}
								vehicle->addRequest(*highestScorer);
								outputFile << "\t\tAdded passenger to vehicle " << vehicleNum << "'s queue at T = " << currentTime << ". Passenger info:" << '\n';
								outputFile << '\t' << "\t\tPickup Location: " << (*highestScorer)->getLocation().first << ", " << (*highestScorer)->getLocation().second << '\n';
								outputFile << '\t' << "\t\tPickup Time: " << (*highestScorer)->getRequestTime() << '\n';
								outputFile << '\t' << "\t\tDestination Location: " << (*highestScorer)->getDestination().first << ", " << (*highestScorer)->getDestination().second << '\n';
								outputFile << '\t' << "\t\tDistance to pickup from next location: " << (*highestScorer)->getDistanceToRequest() << '\n';
								outputFile << '\t' << "\t\tDistance from pickup to dropoff: " << (*highestScorer)->getDistanceOfRequest() << '\n';
								outputFile << '\t' << "\t\tDropoff at T = " << (*highestScorer)->getDistanceOfRequest() + (*highestScorer)->getRequestTime() << '\n';
								outputFile << '\t' << "\t\tRequests available at destination: " << (*highestScorer)->getRequestsAtDestination() << '\n';
								outputFile << '\t' << "\t\tRequest Score: " << topScore << '\n';
								(*highestScorer)->setMatchedToVehicle(true);
								(*highestScorer)->setTimeMatched(i);
								break;
							}
						}
					}

					//handle output after scanning
					if (vehicle->getTopRequest() != nullptr && vehicle->getTopRequest()->getPickedUp()){
						if (!vehicle->getHasPassenger()){
							outputFile << "\t\tVehicle picked up request at Latitude: " << vehicleLocation.first << " and Longitude: " << vehicleLocation.second << " at T = " << currentTime << '\n';
							vehicle->setHasPassenger(true);
							vehicle->addToRoutingLog(i, vehicleLocation);
						}
						else{
							vehicle->setHasPassenger(true);
							outputFile << "\t\tVehicle en route to destination." << '\n';
						}
					}
					else if (vehicle->getTopRequest() != nullptr && !vehicle->getTopRequest()->getPickedUp()){
						if (vehicle->getCurrentLocation().first != vehicle->getTopRequest()->getLocation().first && vehicle->getCurrentLocation().second != vehicle->getTopRequest()->getLocation().second){
							outputFile << "\t\tVehicle en route to pickup." << '\n';
							vehicle->setHasPassenger(false);
						}
						else{
							vehicle->setHasPassenger(false);
							outputFile << "\t\tVehicle waiting at pickup for request." << '\n';
						}
					}
					else if (vehicle->getTopRequest() == nullptr){
						outputFile << "\t\tVehicle idling.";
						vehicle->setHasPassenger(false);
					}

					vehicleNum++;
					outputFile << '\n' << '\n';
				}
				currentTime++;
				outputFile << '\n';
			}


			outputFile << "-------------------------------------------------------------------" << '\n';
			outputFile << "Complete Test Results:" << '\n' << '\n';
			int vehicleNum = 1;
			float distanceWithPassenger = 0;
			float distanceWithoutPassenger = 0;
			float percentUtilization = 100;
			for (Vehicle* vehicle : vehicles[currentTest]){
				outputFile << "Vehicle: " << vehicleNum << '\n';
				outputFile << '\t' << "Distance travelled with a passenger: " << vehicle->getDistanceWithPassenger() << '\n';
				outputFile << '\t' << "Distance travelled without a passenger: " << vehicle->getDistanceWithoutPassenger() << '\n';
				outputFile << '\n' << '\n';
				distanceWithPassenger += vehicle->getDistanceWithPassenger();
				distanceWithoutPassenger += vehicle->getDistanceWithoutPassenger();
				vehicleNum++;
			}
			percentUtilization = (distanceWithPassenger) / (distanceWithoutPassenger + distanceWithPassenger);
			percentUtilization *= 100;
			outputFile << "Fleet utilization is: " << percentUtilization << "%.";
			outputFile.close();
		}
	}

	inline const std::vector<std::string>& getTestNames(){
		return tests;
	}

	inline int getTimesToRun(const std::string& testName){
		return timesToRun[testName];
	}

	inline void prepareToRender(const std::string& testName){
		for (Vehicle* vehicle : vehicles[testName]){
			vehicle->prepareForRendering();
		}
		simTime = 0;
	}

	inline void visualize(float elapsed, const Uint8* input, SDL_Event input2, ShaderProgram* program, const std::string& testName){
		glClear(GL_COLOR_BUFFER_BIT);
		Matrix viewMatrix;
		viewMatrix.Scale((float)managers[testName].getSectionRadius() / (float)managers[testName].getMaxCoords().second, ((float)managers[testName].getSectionRadius() / (float)managers[testName].getMaxCoords().first)/2, 0);
		viewMatrix.Translate(-(managers[testName].getSectionRadius() * 2)-((managers[testName].getMaxCoords().first) / managers[testName].getMaxCoords().second), -(managers[testName].getSectionRadius() * 2) -(managers[testName].getMaxCoords().second / managers[testName].getMaxCoords().first), 0);

		program->setViewMatrix(viewMatrix);
		update(elapsed, testName);
		render(program, elapsed, FRAMES_PER_SECOND, testName);
	}
};

#endif