/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CVehicleNames.cpp
 *  PURPOSE:     Vehicle names class
 *
 *****************************************************************************/

#include <StdInc.h>
#include "CCustomVehicleData.h"

using namespace std;

const char* const szVehicleNameEmpty = "";

struct SVehicleName
{
    const char* szName;
    const char* szName_replaced;  // Compatability
};

static const SFixedArray<SVehicleName, 212> VehicleNames = {{{"Landstalker"},
                                                             {"Bravura"},
                                                             {"Buffalo"},
                                                             {"Linerunner"},
                                                             {"Perennial"},
                                                             {"Sentinel"},
                                                             {"Dumper"},
                                                             {"Fire Truck"},
                                                             {"Trashmaster"},
                                                             {"Stretch"},
                                                             {"Manana"},
                                                             {"Infernus"},
                                                             {"Voodoo"},
                                                             {"Pony"},
                                                             {"Mule"},
                                                             {"Cheetah"},
                                                             {"Ambulance"},
                                                             {"Leviathan"},
                                                             {"Moonbeam"},
                                                             {"Esperanto"},
                                                             {"Taxi"},
                                                             {"Washington"},
                                                             {"Bobcat"},
                                                             {"Mr. Whoopee"},
                                                             {"BF Injection"},
                                                             {"Hunter"},
                                                             {"Premier"},
                                                             {"Enforcer"},
                                                             {"Securicar"},
                                                             {"Banshee"},
                                                             {"Predator"},
                                                             {"Bus"},
                                                             {"Rhino"},
                                                             {"Barracks"},
                                                             {"Hotknife"},
                                                             {"Trailer 1"},
                                                             {"Previon"},
                                                             {"Coach"},
                                                             {"Cabbie"},
                                                             {"Stallion"},
                                                             {"Rumpo"},
                                                             {"RC Bandit"},
                                                             {"Romero"},
                                                             {"Packer"},
                                                             {"Monster 1", "Monster"},
                                                             {"Admiral"},
                                                             {"Squalo"},
                                                             {"Seasparrow"},
                                                             {"Pizzaboy"},
                                                             {"Tram"},
                                                             {"Trailer 2"},
                                                             {"Turismo"},
                                                             {"Speeder"},
                                                             {"Reefer"},
                                                             {"Tropic"},
                                                             {"Flatbed"},
                                                             {"Yankee"},
                                                             {"Caddy"},
                                                             {"Solair"},
                                                             {"Berkley's RC Van"},
                                                             {"Skimmer"},
                                                             {"PCJ-600"},
                                                             {"Faggio"},
                                                             {"Freeway"},
                                                             {"RC Baron"},
                                                             {"RC Raider"},
                                                             {"Glendale"},
                                                             {"Oceanic"},
                                                             {"Sanchez"},
                                                             {"Sparrow"},
                                                             {"Patriot"},
                                                             {"Quadbike"},
                                                             {"Coastguard"},
                                                             {"Dinghy"},
                                                             {"Hermes"},
                                                             {"Sabre"},
                                                             {"Rustler"},
                                                             {"ZR-350"},
                                                             {"Walton"},
                                                             {"Regina"},
                                                             {"Comet"},
                                                             {"BMX"},
                                                             {"Burrito"},
                                                             {"Camper"},
                                                             {"Marquis"},
                                                             {"Baggage"},
                                                             {"Dozer"},
                                                             {"Maverick"},
                                                             {"News Chopper"},
                                                             {"Rancher"},
                                                             {"FBI Rancher"},
                                                             {"Virgo"},
                                                             {"Greenwood"},
                                                             {"Jetmax"},
                                                             {"Hotring Racer"},
                                                             {"Sandking"},
                                                             {"Blista Compact"},
                                                             {"Police Maverick"},
                                                             {"Boxville"},
                                                             {"Benson"},
                                                             {"Mesa"},
                                                             {"RC Goblin"},
                                                             {"Hotring Racer 3"},
                                                             {"Hotring Racer 2"},
                                                             {"Bloodring Banger"},
                                                             {"Rancher Lure"},
                                                             {"Super GT"},
                                                             {"Elegant"},
                                                             {"Journey"},
                                                             {"Bike"},
                                                             {"Mountain Bike"},
                                                             {"Beagle"},
                                                             {"Cropduster"},
                                                             {"Stuntplane"},
                                                             {"Tanker"},
                                                             {"Roadtrain"},
                                                             {"Nebula"},
                                                             {"Majestic"},
                                                             {"Buccaneer"},
                                                             {"Shamal"},
                                                             {"Hydra"},
                                                             {"FCR-900"},
                                                             {"NRG-500"},
                                                             {"HPV1000"},
                                                             {"Cement Truck"},
                                                             {"Towtruck"},
                                                             {"Fortune"},
                                                             {"Cadrona"},
                                                             {"FBI Truck"},
                                                             {"Willard"},
                                                             {"Forklift"},
                                                             {"Tractor"},
                                                             {"Combine Harvester"},
                                                             {"Feltzer"},
                                                             {"Remington"},
                                                             {"Slamvan"},
                                                             {"Blade"},
                                                             {"Freight"},
                                                             {"Streak"},
                                                             {"Vortex"},
                                                             {"Vincent"},
                                                             {"Bullet"},
                                                             {"Clover"},
                                                             {"Sadler"},
                                                             {"Fire Truck Ladder"},
                                                             {"Hustler"},
                                                             {"Intruder"},
                                                             {"Primo"},
                                                             {"Cargobob"},
                                                             {"Tampa"},
                                                             {"Sunrise"},
                                                             {"Merit"},
                                                             {"Utility Van"},
                                                             {"Nevada"},
                                                             {"Yosemite"},
                                                             {"Windsor"},
                                                             {"Monster 2"},
                                                             {"Monster 3"},
                                                             {"Uranus"},
                                                             {"Jester"},
                                                             {"Sultan"},
                                                             {"Stratum"},
                                                             {"Elegy"},
                                                             {"Raindance"},
                                                             {"RC Tiger"},
                                                             {"Flash"},
                                                             {"Tahoma"},
                                                             {"Savanna"},
                                                             {"Bandito"},
                                                             {"Freight Train Flatbed"},
                                                             {"Streak Train Trailer"},
                                                             {"Kart"},
                                                             {"Mower"},
                                                             {"Dune"},
                                                             {"Sweeper"},
                                                             {"Broadway"},
                                                             {"Tornado"},
                                                             {"AT-400"},
                                                             {"DFT-30"},
                                                             {"Huntley"},
                                                             {"Stafford"},
                                                             {"BF-400"},
                                                             {"Newsvan"},
                                                             {"Tug"},
                                                             {"Trailer (Tanker Commando)"},
                                                             {"Emperor"},
                                                             {"Wayfarer"},
                                                             {"Euros"},
                                                             {"Hotdog"},
                                                             {"Club"},
                                                             {"Box Freight"},
                                                             {"Trailer 3"},
                                                             {"Andromada"},
                                                             {"Dodo"},
                                                             {"RC Cam"},
                                                             {"Launch"},
                                                             {"Police LS", "Police"},
                                                             {"Police SF"},
                                                             {"Police LV"},
                                                             {"Police Ranger", "Ranger"},
                                                             {"Picador"},
                                                             {"S.W.A.T."},
                                                             {"Alpha"},
                                                             {"Phoenix"},
                                                             {"Glendale Damaged"},
                                                             {"Sadler", "Sadler Damaged"},
                                                             {"Baggage Trailer (covered)"},
                                                             {"Baggage Trailer (Uncovered)"},
                                                             {"Trailer (Stairs)"},
                                                             {"Boxville Mission"},
                                                             {"Farm Trailer"},
                                                             {"Street Clean Trailer"}}};

static const SFixedArray<SVehicleName, 10> VehicleTypes = {
    {{"Automobile"}, {"Plane"}, {"Bike"}, {"Helicopter"}, {"Boat"}, {"Train"}, {"Trailer"}, {"BMX"}, {"Monster Truck"}, {"Quad"}}};

static const SFixedArray<unsigned char, 212> ucVehicleTypes = {
    0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0,
    0, 8, 0, 4, 3, 2, 5, 6, 0, 4, 4, 4, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 3, 0, 0, 2, 3, 0, 9, 4, 4, 0, 0, 1, 0, 0, 0, 0, 7, 0, 0, 4, 0,
    0, 3, 3, 0, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 7, 7, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 1, 0, 0, 8, 8, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 5, 5, 0,
    0, 8, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 6, 0, 2, 0, 0, 0, 5, 6, 1, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 0, 6, 6};

bool CVehicleNames::IsValidModel(unsigned long ulModel)
{
    if (ulModel >= 400 && ulModel <= 611)
        return true;
    if (ulModel >= CUSTOM_VEHICLE_MIN && ulModel <= CUSTOM_VEHICLE_MAX)
        return true;
    return false;
}

bool CVehicleNames::IsModelTrailer(unsigned long ulModel)
{
    // Stock trailers
    if (ulModel == 435 || ulModel == 450 || ulModel == 591 || ulModel == 606 || ulModel == 607 || ulModel == 584 || ulModel == 608 || ulModel == 610 ||
        ulModel == 611)
        return true;
    // Custom trailers
    if (ulModel >= CUSTOM_VEHICLE_MIN && ulModel <= CUSTOM_VEHICLE_MAX)
        return GetCustomVehicleType(static_cast<std::uint32_t>(ulModel)) == 11;
    return false;
}

const char* CVehicleNames::GetVehicleName(unsigned long ulModel)
{
    // Stock vehicles
    if (ulModel >= 400 && ulModel <= 611)
        return VehicleNames[ulModel - 400].szName;

    // Custom FLA vehicles
    const char* customName = GetCustomVehicleName(static_cast<std::uint32_t>(ulModel));
    if (customName)
        return customName;

    return szVehicleNameEmpty;
}

unsigned int CVehicleNames::GetVehicleModel(const char* szName)
{
    // If the specified string was empty, return 0
    if (szName[0] == 0)
        return 0;

    // Look in stock table
    for (unsigned int i = 0; i < NUMELMS(VehicleNames); i++)
    {
        if (stricmp(szName, VehicleNames[i].szName) == 0 || (VehicleNames[i].szName_replaced && stricmp(szName, VehicleNames[i].szName_replaced) == 0))
            return i + 400;
    }

    // Look in custom vehicle table
    std::uint32_t customModel = GetCustomVehicleModel(szName);
    if (customModel != 0)
        return customModel;

    return 0;
}

const char* CVehicleNames::GetVehicleTypeName(unsigned long ulModel)
{
    // Stock vehicles: direct array lookup
    if (ulModel >= 400 && ulModel <= 611 && ((ulModel - 400) < NUMELMS(ucVehicleTypes)))
    {
        int iVehicleType = ucVehicleTypes[ulModel - 400];
        return VehicleTypes[iVehicleType].szName;
    }

    // Custom FLA vehicles: use generated type map
    std::uint8_t rawType = GetCustomVehicleType(static_cast<std::uint32_t>(ulModel));
    if (rawType != 0xFF)
    {
        // Map GTA type codes to VehicleTypes array indices
        // VehicleTypes: 0=Automobile, 1=Plane, 2=Bike, 3=Helicopter, 4=Boat, 5=Train, 6=Trailer, 7=BMX, 8=Monster Truck, 9=Quad
        switch (rawType)
        {
            case 0:  return VehicleTypes[0].szName;  // car -> Automobile
            case 1:  return VehicleTypes[8].szName;  // mtruck -> Monster Truck
            case 2:  return VehicleTypes[9].szName;  // quad -> Quad
            case 3:  return VehicleTypes[3].szName;  // heli -> Helicopter
            case 4:  return VehicleTypes[1].szName;  // plane -> Plane
            case 5:  return VehicleTypes[4].szName;  // boat -> Boat
            case 6:  return VehicleTypes[5].szName;  // train -> Train
            case 9:  return VehicleTypes[2].szName;  // bike -> Bike
            case 10: return VehicleTypes[7].szName;  // bmx -> BMX
            case 11: return VehicleTypes[6].szName;  // trailer -> Trailer
        }
    }

    return szVehicleNameEmpty;
}