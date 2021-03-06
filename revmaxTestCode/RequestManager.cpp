#include "RequestManager.h"
#include "RideRequest.h"
//#include "EventVenue.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include "mathHelper.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288419
#endif


RequestManager::RequestManager()
{
}

RequestManager::RequestManager(int sectionRadius, int latitudeMax, int longitudeMax, int latitudeMin, int longitudeMin) : sectionRadius(sectionRadius), latitudeMax(latitudeMax), longitudeMax(longitudeMax), latitudeMin(latitudeMin), longitudeMin(longitudeMin) {
	initializeRequestMap();
}

RequestManager::~RequestManager()
{
}

void RequestManager::initializeRequestMap(){
	normalizeCoordinates();
	for (int i = latitudeMin; i <= latitudeMax; i += sectionRadius){
		for (int j = longitudeMin; j <= longitudeMax; j += sectionRadius){
			requestMap[i][j] = {};
		}
	}
}

void RequestManager::normalizeCoordinates(){
	if (latitudeMin % sectionRadius != 0){
		latitudeMin -= latitudeMin % sectionRadius;
	}
	if (longitudeMin % sectionRadius != 0){
		longitudeMin -= longitudeMin % sectionRadius;
	}

	if ((int)latitudeMax % sectionRadius != 0){
		latitudeMax += latitudeMax % sectionRadius;
	}
	if ((int)longitudeMax % sectionRadius != 0){
		longitudeMax += longitudeMax % sectionRadius;
	}
}

void RequestManager::addRequest(RideRequest* request){
	int latitudeToUse = (int)request->getLocation().first;
	int longitudeToUse = (int)request->getLocation().second;
	if (latitudeToUse % sectionRadius > sectionRadius / 2){
		latitudeToUse += sectionRadius - (latitudeToUse % sectionRadius);
	}
	else if (latitudeToUse % sectionRadius <= sectionRadius / 2){
		latitudeToUse -= latitudeToUse % sectionRadius;
	}

	if (longitudeToUse % sectionRadius > sectionRadius / 2){
		longitudeToUse += sectionRadius - (longitudeToUse % sectionRadius);
	}
	else if (longitudeToUse % sectionRadius <= sectionRadius / 2){
		longitudeToUse -= longitudeToUse % sectionRadius;
	}

	if (latitudeToUse < latitudeMin){
		latitudeToUse = latitudeMin;
	}
	else if (latitudeToUse > latitudeMax){
		latitudeToUse = latitudeMax;
	}

	if (longitudeToUse < longitudeMin){
		longitudeToUse = longitudeMin;
	}
	else if (longitudeToUse > longitudeMax){
		longitudeToUse = longitudeMax;
	}

	requestMap[latitudeToUse][longitudeToUse].push_back(request);
	allRideRequests.push_back(request);
}

//void RequestManager::addVenue(EventVenue* venue){
//	int latitudeToUse = (int)venue->getLocation().first;
//	int longitudeToUse = (int)venue->getLocation().second;
//	if (latitudeToUse % sectionRadius > sectionRadius / 2){
//		latitudeToUse += sectionRadius - (latitudeToUse % sectionRadius);
//	}
//	else if (latitudeToUse % sectionRadius <= sectionRadius / 2){
//		latitudeToUse -= latitudeToUse % sectionRadius;
//	}
//
//	if (longitudeToUse % sectionRadius > sectionRadius / 2){
//		longitudeToUse += sectionRadius - (longitudeToUse % sectionRadius);
//	}
//	else if (longitudeToUse % sectionRadius <= sectionRadius / 2){
//		longitudeToUse -= longitudeToUse % sectionRadius;
//	}
//	venueMap[latitudeToUse][longitudeToUse].push_back(venue);
//	allVenues.push_back(venue);
//}

std::vector<RideRequest*>& RequestManager::getRequestsAtLocation(std::pair<long, long> location){
	int latitudeToUse = (int)location.first;
	int longitudeToUse = (int)location.second;

	//This could later be re-worked into ranged SQL queries.
	if (latitudeToUse % sectionRadius > sectionRadius / 2){
		latitudeToUse += sectionRadius - (latitudeToUse % sectionRadius);
	}
	else if (latitudeToUse % sectionRadius <= sectionRadius / 2){
		latitudeToUse -= latitudeToUse % sectionRadius;
	}

	if (longitudeToUse % sectionRadius > sectionRadius / 2){
		longitudeToUse += sectionRadius - (longitudeToUse % sectionRadius);
	}
	else if (longitudeToUse % sectionRadius <= sectionRadius / 2){
		longitudeToUse -= longitudeToUse % sectionRadius;
	}

	if (latitudeToUse < latitudeMin){
		latitudeToUse = latitudeMin;
	}
	else if (latitudeToUse > latitudeMax){
		latitudeToUse = latitudeMax;
	}

	if (longitudeToUse < longitudeMin){
		longitudeToUse = longitudeMin;
	}
	else if (longitudeToUse > longitudeMax){
		longitudeToUse = longitudeMax;
	}

	return requestMap[latitudeToUse][longitudeToUse];
}

void RequestManager::setSectionRadius(int sectionRadius){
	this->sectionRadius = sectionRadius;
}

void RequestManager::setLatitudeMax(int latitudeMax){
	this->latitudeMax = latitudeMax;
}

void RequestManager::setLongitudeMax(int longitudeMax){
	this->longitudeMax = longitudeMax;
}

void RequestManager::setLatitudeMin(int latitudeMin){
	this->latitudeMin = latitudeMin;
}

void RequestManager::setLongitudeMin(int longitudeMin){
	this->longitudeMin = longitudeMin;
}

std::pair<int, int> RequestManager::getMinCoords(){
	return std::make_pair(latitudeMin, longitudeMin);
}

std::pair<int, int> RequestManager::getMaxCoords(){
	return std::make_pair(latitudeMax, longitudeMax);
}

int RequestManager::getNumberOfRequestsAtLocation(std::pair<long, long> location, int time, int timeRadius){
	int latitudeToUse = (int)location.first;
	int longitudeToUse = (int)location.second;
	int requestCount = 0;

	//This could later be re-worked into ranged SQL queries.
	if (latitudeToUse % sectionRadius > sectionRadius / 2){
		latitudeToUse += sectionRadius - (latitudeToUse % sectionRadius);
	}
	else if (latitudeToUse % sectionRadius <= sectionRadius / 2){
		latitudeToUse -= latitudeToUse % sectionRadius;
	}

	if (longitudeToUse % sectionRadius > sectionRadius / 2){
		longitudeToUse += sectionRadius - (longitudeToUse % sectionRadius);
	}
	else if (longitudeToUse % sectionRadius <= sectionRadius / 2){
		longitudeToUse -= longitudeToUse % sectionRadius;
	}
	for (size_t i = 0; i < requestMap[latitudeToUse][longitudeToUse].size(); i++){
		if (requestMap[latitudeToUse][longitudeToUse][i]->getRequestTime() == time){
			requestCount++;
		}
	}

	//for (size_t i = 0; i < venueMap[latitudeToUse][longitudeToUse].size(); i++){
	//	requestCount += venueMap[latitudeToUse][longitudeToUse][i]->getProjectedRequests(time, timeRadius);
	//}

	return requestCount;
}

void RequestManager::setLineTexture(Texture* texture){
	lineTexture = texture;
}

void RequestManager::setRequestTexture(Texture* texture){
	requestTexture = texture;
}

//void RequestManager::setVenueTexture(Texture* texture){
//	venueTexture = texture;
//}

void RequestManager::render(ShaderProgram* program, float time, float timeRadius, float scaleX, float scaleY){
	std::vector<GLfloat> objectVertices;
	std::vector<GLfloat> textureCoordinates;
	for (int i = longitudeMin; i <= longitudeMax; i += sectionRadius){
		for (int j = latitudeMin; j <= latitudeMax; j += sectionRadius){
			modelMatrix.identity();
			modelMatrix.Translate(i, j, 0);
			modelMatrix.Scale(sectionRadius, 0.25, 0);
			objectVertices = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };

			textureCoordinates = gridTexture->getTextureCoordinates();
			glBindTexture(GL_TEXTURE_2D, gridTexture->getTextureID());
			program->setModelMatrix(modelMatrix);

			glEnableVertexAttribArray(program->positionAttribute);
			glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, objectVertices.data());

			glEnableVertexAttribArray(program->texCoordAttribute);
			glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordinates.data());

			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(program->positionAttribute);
			glDisableVertexAttribArray(program->texCoordAttribute);

			modelMatrix.setScale(1, 1, 0);
			//modelMatrix.Translate(sectionRadius, 0, 0);
			modelMatrix.setScale(0.5, sectionRadius, 0);
			objectVertices = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };

			textureCoordinates = gridTexture->getTextureCoordinates();
			glBindTexture(GL_TEXTURE_2D, gridTexture->getTextureID());
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
	for (RideRequest* request : allRideRequests){
		if ((request->getMatchedToVehicle() && time >= request->getRequestTime() - timeRadius) || (!request->getMatchedToVehicle() && (time - timeRadius) <= request->getRequestTime() && request->getRequestTime() <= (time + timeRadius))){
			modelMatrix.identity();
			modelMatrix.Translate(request->getLocation().second, request->getLocation().first, 0);
			//modelMatrix.setPosition(1.0, -2.0, 0.0);
			modelMatrix.Scale(scaleX, scaleY, 0);
			//modelMatrix.Rotate(3.14/4);
			objectVertices = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };

			textureCoordinates = requestTexture->getTextureCoordinates();
			glBindTexture(GL_TEXTURE_2D, requestTexture->getTextureID());
			program->setModelMatrix(modelMatrix);

			glEnableVertexAttribArray(program->positionAttribute);
			glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, objectVertices.data());

			glEnableVertexAttribArray(program->texCoordAttribute);
			glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordinates.data());

			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisableVertexAttribArray(program->positionAttribute);
			glDisableVertexAttribArray(program->texCoordAttribute);
		}
		//if (request->getMatchedToVehicle() && time >= request->getRequestTime() && time <= request->getRequestTime() + request->getDistanceOfRequest() + timeRadius){
		//	modelMatrix.identity();
		//	modelMatrix.Translate(request->getDestination().second, request->getDestination().first + 0.5, 0);
		//	modelMatrix.Scale(scaleX, scaleY * 2, 0);
		//	objectVertices = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
		//	// {-x, -y}, {x, -y}, {x, y}
		//	// {-x, -y}, {x, y}, {-x, y}

		//	textureCoordinates = destinationTexture->getTextureCoordinates();
		//	glBindTexture(GL_TEXTURE_2D, destinationTexture->getTextureID());
		//	program->setModelMatrix(modelMatrix);

		//	glEnableVertexAttribArray(program->positionAttribute);
		//	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, objectVertices.data());

		//	glEnableVertexAttribArray(program->texCoordAttribute);
		//	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordinates.data());

		//	glDrawArrays(GL_TRIANGLES, 0, 6);
		//	glDisableVertexAttribArray(program->positionAttribute);
		//	glDisableVertexAttribArray(program->texCoordAttribute);
		//	
		//	float lineWidth = 0.5;

		//	float destX = request->getDestination().second;
		//	float destY = request->getDestination().first;

		//	float locX = request->getLocation().second;
		//	float locY = request->getLocation().first;

		//	float dist = pythagDistance(locX, locY, destX, destY);

		//	// Triangle Right: {-x, -y}, {x, -y}, {x, y}
		//	// Triangle Left: {-x, -y}, {x, y}, {-x, y}
		//	float centerX = (locX + destX) / 2;
		//	float centerY = (locY + destY) / 2;

		//	//float angle = atan2f(destY - locY, destX - locX);
		//	float angle = abs(acosf((destY - locY) / dist));
		//	while (angle >= M_PI){
		//		angle -= M_PI;
		//	}
		//	//if (angle > 3.14159 / 4 && angle < 3.14159 / 3){
		//	//	angle *= -1;
		//	//}
		//	if (destX > locX){
		//		angle *= -1;
		//	}
		//	textureCoordinates = lineTexture->getTextureCoordinates();
		//	glBindTexture(GL_TEXTURE_2D, lineTexture->getTextureID());
		//	modelMatrix.identity();
		//	modelMatrix.Translate(centerX, centerY, 0);
		//	modelMatrix.Rotate(angle);
		//	modelMatrix.Scale(lineWidth, dist, 0);
		//	program->setModelMatrix(modelMatrix);

		//	glEnableVertexAttribArray(program->positionAttribute);
		//	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, objectVertices.data());

		//	glEnableVertexAttribArray(program->texCoordAttribute);
		//	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordinates.data());

		//	glDrawArrays(GL_TRIANGLES, 0, 6);
		//	glDisableVertexAttribArray(program->positionAttribute);
		//	glDisableVertexAttribArray(program->texCoordAttribute);
		//}
	}

	//for (EventVenue* venue : allVenues){
	//	modelMatrix.identity();
	//	modelMatrix.Translate(venue->getLocation().second, venue->getLocation().first, 0);
	//	//modelMatrix.setPosition(1.0, -2.0, 0.0);
	//	modelMatrix.Scale(scaleX, scaleY, 0);
	//	//modelMatrix.Rotate(3.14 / 4);
	//	objectVertices = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };

	//	textureCoordinates = venueTexture->getTextureCoordinates();
	//	glBindTexture(GL_TEXTURE_2D, venueTexture->getTextureID());
	//	program->setModelMatrix(modelMatrix);

	//	glEnableVertexAttribArray(program->positionAttribute);
	//	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, objectVertices.data());

	//	glEnableVertexAttribArray(program->texCoordAttribute);
	//	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textureCoordinates.data());

	//	glDrawArrays(GL_TRIANGLES, 0, 6);
	//	glDisableVertexAttribArray(program->positionAttribute);
	//	glDisableVertexAttribArray(program->texCoordAttribute);
	//}
}

int RequestManager::getSectionRadius(){
	return sectionRadius;
}

void RequestManager::freeMemory(){
	requestMap.clear();
	//venueMap.clear();
	lineTexture = nullptr;
	//venueTexture = nullptr;
	requestTexture = nullptr;
	destinationTexture = nullptr;
	for (int i = 0; i < allRideRequests.size(); i++){
		delete allRideRequests[i];
		allRideRequests[i] = nullptr;
	}
	allRideRequests.clear();
	//for (int i = 0; i < allVenues.size(); i++){
	//	delete allVenues[i];
	//	allVenues[i] = nullptr;
	//}
	//allRideRequests.clear();
}

void RequestManager::setDestinationTexture(Texture* texture){
	destinationTexture = texture;
}