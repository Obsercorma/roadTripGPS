import pandas as pd 
import folium
from math import cos,sin,radians,acos,sqrt,ceil
from datetime import datetime
#mark = folium.Marker(location=[45.31952448345068, 6.993577823564601], popup="Votre position",icon=folium.Icon(color='blue', icon='ok-sign'))
points = []
groupPoints = []
posGroupPoints = 0
coordsVectors = []
sommeDistance = 0.0
sommeCelcius = 0.0
celciusToRemove = 0
stateMarker = ""
markerPoints = []
arrayPosCoordsVectors = [0]
posPoints = 0
# colorAddOp = 0x00c
# startColor = 0x00c

COLORS = ['green', 'red', 'blue', 'cadetblue', 'pink', 'darkblue', 'darkred', 'lightgreen', 'lightblue', 'black', 'purple', 'darkgreen', 'lightgray', 'darkpurple', 'orange', 'white', 'beige', 'gray', 'lightred']
startMarkerColor = COLORS[2]
startLineColor = COLORS[0]

def calcDistance_XOnly(pointFrom:tuple,pointTo:tuple)-> float:
    return (acos(sin(radians(pointFrom[0]))*sin(radians(pointTo[0]))
    +cos(radians(pointFrom[0]))*cos(radians(pointTo[0]))*cos(radians(pointFrom[1]-pointTo[1])))
    *6371)
def calcDistance(pointFrom:list,pointTo:list)-> float:
    # pointFrom, pointTo: [0]->latitude, [1]->longitude, [2]->altitude
    calcVarX:float = acos(sin(radians(pointFrom[0]))*sin(radians(pointTo[0]))+cos(radians(pointFrom[0]))*cos(radians(pointTo[0]))*cos(radians(pointTo[1]-pointFrom[1])))*6371
    calcVarY:float = (pointTo[2]-pointFrom[2])/1000
    #print(calcVarY)
    return sqrt((calcVarX**2)+(calcVarY**2))

def calcDeniv(pointsCalc:list)->float:
    altPoints = [p[2] for p in pointsCalc]
    maxDeniv = max(altPoints)
    minDeniv = min(altPoints)
    return maxDeniv-minDeniv
def subString(text:str,startAt:int,stopChar:str)->str:
    return text[startAt:text.index(stopChar)]

def timeHourOffset(timeStr:str,hourOffset:int)->str:
    retVal = int(timeStr[0:2 if len(timeStr)>4 else 1],10)
    print(f"TIME:{retVal}")
    if retVal>=24:
        return f"{retVal-24}"+timeStr[2:]
    return (f"{retVal+hourOffset}"+(':' if len(timeStr)<=4 else '')+timeStr[2:])

fileOpen = input("Module file to open:>") #23072023_data.html, 23072023_DAT01.TXT
fileSaving = input("File to save:>")

with open(f"./examples/{fileOpen}") as file:
    for data in file.readlines():
        if "#IGNORE:" in data:
            continue
        if("RT_P" in data ) or ("RT_R" in data) or ("DEST_R" in data):
            stateMarker = data
            continue
        if (not "LT_I" in data) and (not "LG_I" in data) and (not "AT_I" in data):
            posPoints+=1
            tmpData = data.split(";")
            if not "#NV" in tmpData[5]:
                valTemp = float(subString(tmpData[5],0,'#'))
                if valTemp <= 0.0:
                    celciusToRemove+=1
            else:
                valTemp = 0.0
                celciusToRemove+=1
            points.append(
                (float(tmpData[0]),float(tmpData[1]),float(tmpData[2]), tmpData[3], tmpData[4], valTemp)
            )
            if stateMarker != "":
                translatedState = "Pause" if "RT_P" in stateMarker else "Reprise" if "RT_R" in stateMarker else "Arrivée"
                markerPoints.append(folium.Marker(location=[float(tmpData[0]),float(tmpData[1])], popup=f"{translatedState} à {timeHourOffset(subString(tmpData[4],0,'#'),2)}", icon=folium.Icon(color=startMarkerColor, icon='flag' if translatedState == "Arrivée" else 'pause')))
                print(f"COLOR:{startMarkerColor}")
                posMarkerColor = COLORS.index(startMarkerColor)+1
                if posMarkerColor == len(COLORS):
                    posMarkerColor = 0
                startMarkerColor = COLORS[posMarkerColor]
                stateMarker = ""
                if translatedState == "Arrivée":
                    arrayPosCoordsVectors.append(posPoints)
                    groupPoints.append(points)

world = folium.Map(
    zoom_start=15,
    # location=[points[int(len(points)/2)][0], points[int(len(points)/2)][1]],
    location=[points[(len(points)//2)][0],points[(len(points)//2)][1]],
    max_zoom=15
)
startMarker = folium.Marker(location=[points[0][0],points[0][1]], popup=f"Début à {timeHourOffset(subString(points[0][4],0,'#'),2)}", icon=folium.Icon(color='green', icon='off'))
endMarker = folium.Marker(location=[points[-1][0],points[-1][1]], popup=f"Fin à {timeHourOffset(subString(points[-1][4],0,'#'),2)}", icon=folium.Icon(color='red', icon='off'))
sumPoints = 0
groupCoordsVectors = []
posGroupVectors = 0
sumPoints=len(points)
for p in range(0,sumPoints):
    #for psub in range(0,len(p)):
    coordsVectors.append(points[p][0])
    coordsVectors.append(points[p][1])
        #sommeDistance += calcDistance_XOnly((points[p-1][0],points[p-1][1]),(points[p][0],points[p][1]))
    sommeCelcius += points[p][5]
        #else: celciusToRemove += 1

    if p+1 < sumPoints:
        sommeDistance += calcDistance([points[p][0],points[p][1],points[p][2]],[points[p+1][0],points[p+1][1],points[p+1][2]])
    """mark = folium.Marker(
        location=[p[0],p[1]],
        popup=f"Altitude:{p[2]}, Date:{p[3][0:p[3].index('#')]}, Time:{p[4][0:p[4].index('#')]}, {p[5]}°C",
        icon=folium.Icon(color="red", icon="ok-sign")
    )
    mark.add_to(world)"""
    groupCoordsVectors.append(coordsVectors)
    coordsVectors = []

sommeCelcius /= (sumPoints-celciusToRemove)

print(round(sommeDistance,2), round(sommeCelcius,2))
print(f"NUMBER OF INVALID DHT DATA: {celciusToRemove}/{len(points)}")
arrayPosCoordsVectors.append(sumPoints-1)
grpPlElements = folium.FeatureGroup(name="Parcours")
for posGroupVectors in range(0,len(arrayPosCoordsVectors)-1):
    deniv = calcDeniv(points)
    locations = groupCoordsVectors[arrayPosCoordsVectors[posGroupVectors]:arrayPosCoordsVectors[posGroupVectors+1]]
    if locations == []:
        continue
    folium.PolyLine(
        locations=locations, 
        tooltip=f"""Trajet: {subString(points[0][3],0,'#')}
        <br/>Parcours {sommeDistance:.2f}km
        <br/>Moyenne Celcius: {sommeCelcius:.2f}°C
        <br/>Deniv+ {deniv:.2f}m""", color=startLineColor).add_to(grpPlElements)
    posColorLineStarting = COLORS.index(startLineColor)+1
    if posColorLineStarting >= len(COLORS):
        posColorLineStarting = 0
    startLineColor = COLORS[posColorLineStarting]

startMarker.add_to(world)
endMarker.add_to(world)
grpPlElements.add_to(world)

for m in markerPoints:
    m.add_to(world)

world.options["zoomControl"] = True
world.save(fileSaving)
