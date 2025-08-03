import geopandas as gpd
from shapely.geometry import Point

# Load the WWF ecoregions data
gdf = gpd.read_file("./assets/wwf_terr_ecos.shp")

# Example coordinates: Pralognan-la-Vanoise (Alps)
lat = 45.3784
lon = 6.7198
point = Point(lon, lat)

# Find which biome the point is in
match = gdf[gdf.contains(point)]

if not match.empty:
    biome = match.iloc[0]["BIOME_NAME"]  # or ECO_NAME if more detailed
    print({"village": "Pralognan", "biome": biome})
else:
    print("Biome not found")