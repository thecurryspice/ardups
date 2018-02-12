#define MOSPIN 3
#define BATTCHRGPIN 4
#define CONSTVOLT 13
#define CURRPIN A0
#define BATTVOLTPIN A1
#define SUPPLYVOLTPIN A2
#define CURRSENS analogRead(CURRPIN)
#define BATTVOLT analogRead(BATTVOLTPIN)
#define SUPPLYVOLT analogRead(SUPPLYVOLTPIN)


// DEMANDIND is actually switching of free-running to CV mode