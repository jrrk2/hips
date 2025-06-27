// MessierCatalog.h - Messier objects converted to SkyPosition format
#ifndef MESSIERCATALOG_H
#define MESSIERCATALOG_H

#include "ProperHipsClient.h"
#include <QStringList>
#include <QMap>

// Object type enumeration
enum class MessierObjectType {
    GLOBULAR_CLUSTER,
    OPEN_CLUSTER, 
    NEBULA,
    PLANETARY_NEBULA,
    SUPERNOVA_REMNANT,
    GALAXY,
    GALAXY_CLUSTER,
    DOUBLE_STAR,
    ASTERISM,
    STAR_CLOUD,
    OTHER
};

// Constellation enumeration
enum class Constellation {
    ANDROMEDA, AQUARIUS, AURIGA, CANCER, CANES_VENATICI, CANIS_MAJOR,
    CAPRICORNUS, CASSIOPEIA, CETUS, COMA_BERENICES, CYGNUS, DRACO,
    GEMINI, HERCULES, HYDRA, LEO, LEPUS, LYRA, MONOCEROS, OPHIUCHUS,
    ORION, PEGASUS, PERSEUS, PISCES, PUPPIS, SAGITTA, SAGITTARIUS,
    SCORPIUS, SCUTUM, SERPENS, TAURUS, TRIANGULUM, URSA_MAJOR,
    VIRGO, VULPECULA
};

// Messier object structure
struct MessierObject {
    int id;
    QString name;
    QString common_name;
    MessierObjectType object_type;
    Constellation constellation;
    SkyPosition sky_position;  // Converted from RA hours/Dec degrees
    float magnitude;
    float distance_kly;
    QSizeF size_arcmin;  // width, height in arcminutes
    QString description;
    QString best_viewed;
    bool has_been_imaged;  // From your imaged list
};

class MessierCatalog {
public:
    static QList<MessierObject> getAllObjects();
    static MessierObject getObjectById(int id);
    static QList<MessierObject> getImagedObjects();
    static QList<MessierObject> getObjectsByType(MessierObjectType type);
    static QList<MessierObject> getObjectsByConstellation(Constellation constellation);
    static QStringList getObjectNames();
    static QString objectTypeToString(MessierObjectType type);
    static QString constellationToString(Constellation constellation);
    
private:
    static QList<MessierObject> m_catalog;
    static void initializeCatalog();
    static SkyPosition createSkyPosition(double ra_hours, double dec_degrees, 
                                       const QString& name, const QString& description);
};

// Convert RA hours to degrees  
inline double raHoursToDegrees(double ra_hours) {
    return ra_hours * 15.0;  // 1 hour = 15 degrees
}

// Initialize the catalog with converted coordinates
QList<MessierObject> MessierCatalog::m_catalog;

void MessierCatalog::initializeCatalog() {
    if (!m_catalog.isEmpty()) return;
    
    // Set of imaged objects for quick lookup
    QSet<QString> imaged_objects = {
        "M1", "M3", "M13", "M16", "M17", "M27", "M45", "M51", 
        "M74", "M81", "M101", "M106", "M109"
    };
    
    m_catalog = {
        {1, "M1", "Crab Nebula", MessierObjectType::SUPERNOVA_REMNANT, Constellation::TAURUS,
         createSkyPosition(5.575556, 22.013333, "M1", "Crab Nebula - Supernova remnant from 1054 AD"),
         20.0f, 6.5f, QSizeF(6.0, 4.0), "Remains of a supernova observed in 1054 AD", "Winter",
         imaged_objects.contains("M1")},
         
        {2, "M2", "", MessierObjectType::GLOBULAR_CLUSTER, Constellation::AQUARIUS,
         createSkyPosition(21.557506, -0.823250, "M2", "Globular cluster in Aquarius"),
         6.2f, 37.5f, QSizeF(16.0, 16.0), "One of the richest and most compact globular clusters", "Autumn",
         imaged_objects.contains("M2")},
         
        {3, "M3", "", MessierObjectType::GLOBULAR_CLUSTER, Constellation::CANES_VENATICI,
         createSkyPosition(13.703228, 28.377278, "M3", "Globular cluster in Canes Venatici"),
         6.4f, 33.9f, QSizeF(18.0, 18.0), "Contains approximately 500,000 stars", "Spring",
         imaged_objects.contains("M3")},
         
        {13, "M13", "Hercules Globular Cluster", MessierObjectType::GLOBULAR_CLUSTER, Constellation::HERCULES,
         createSkyPosition(16.694898, 36.461319, "M13", "Great Globular Cluster in Hercules"),
         5.8f, 22.2f, QSizeF(20.0, 20.0), "Contains several hundred thousand stars", "Summer",
         imaged_objects.contains("M13")},
         
        {16, "M16", "Eagle Nebula", MessierObjectType::OPEN_CLUSTER, Constellation::SERPENS,
         createSkyPosition(18.312500, -13.791667, "M16", "Eagle Nebula with Pillars of Creation"),
         6.0f, 7.0f, QSizeF(35.0, 28.0), "Contains the famous 'Pillars of Creation'", "Summer",
         imaged_objects.contains("M16")},
         
        {17, "M17", "Omega Nebula", MessierObjectType::NEBULA, Constellation::SAGITTARIUS,
         createSkyPosition(18.346389, -16.171667, "M17", "Omega/Swan/Horseshoe Nebula"),
         20.0f, 5.0f, QSizeF(11.0, 11.0), "Also known as the Swan Nebula or Horseshoe Nebula", "Summer",
         imaged_objects.contains("M17")},
         
        {27, "M27", "Dumbbell Nebula", MessierObjectType::PLANETARY_NEBULA, Constellation::VULPECULA,
         createSkyPosition(19.993434, 22.721198, "M27", "Dumbbell Nebula - planetary nebula"),
         14.1f, 1.2f, QSizeF(8.0, 5.7), "One of the brightest planetary nebulae in the sky", "Summer",
         imaged_objects.contains("M27")},
         
        {31, "M31", "Andromeda Galaxy", MessierObjectType::GALAXY, Constellation::ANDROMEDA,
         createSkyPosition(0.712314, 41.268750, "M31", "Andromeda Galaxy - nearest major galaxy"),
         3.4f, 2500.0f, QSizeF(178.0, 63.0), "The nearest major galaxy to the Milky Way", "Autumn",
         imaged_objects.contains("M31")},
         
        {42, "M42", "Orion Nebula", MessierObjectType::NEBULA, Constellation::ORION,
         createSkyPosition(5.588139, -5.391111, "M42", "Great Orion Nebula"),
         20.0f, 1.3f, QSizeF(85.0, 60.0), "One of the brightest nebulae visible to the naked eye", "Winter",
         imaged_objects.contains("M42")},
         
        {45, "M45", "Pleiades", MessierObjectType::OPEN_CLUSTER, Constellation::TAURUS,
         createSkyPosition(3.773333, 24.113333, "M45", "Pleiades - The Seven Sisters"),
         20.0f, 0.4f, QSizeF(110.0, 110.0), "The Seven Sisters, visible to naked eye", "Winter",
         imaged_objects.contains("M45")},
         
        {51, "M51", "Whirlpool Galaxy", MessierObjectType::GALAXY, Constellation::CANES_VENATICI,
         createSkyPosition(13.497972, 47.195258, "M51", "Whirlpool Galaxy - classic spiral"),
         8.4f, 23000.0f, QSizeF(11.2, 6.9), "A classic example of a spiral galaxy", "Spring",
         imaged_objects.contains("M51")},
         
        {57, "M57", "Ring Nebula", MessierObjectType::PLANETARY_NEBULA, Constellation::LYRA,
         createSkyPosition(18.893082, 33.029134, "M57", "Ring Nebula in Lyra"),
         15.8f, 2.3f, QSizeF(1.4, 1.0), "A classic planetary nebula with a ring-like appearance", "Summer",
         imaged_objects.contains("M57")},
         
        {74, "M74", "", MessierObjectType::GALAXY, Constellation::PISCES,
         createSkyPosition(1.611596, 15.783641, "M74", "Face-on spiral galaxy in Pisces"),
         9.5f, 32.0f, QSizeF(10.2, 9.5), "A face-on spiral galaxy with well-defined arms", "Autumn",
         imaged_objects.contains("M74")},
         
        {81, "M81", "Bode's Galaxy", MessierObjectType::GALAXY, Constellation::URSA_MAJOR,
         createSkyPosition(9.925881, 69.065295, "M81", "Bode's Galaxy - grand design spiral"),
         6.9f, 11.8f, QSizeF(26.9, 14.1), "A grand design spiral galaxy", "Spring",
         imaged_objects.contains("M81")},
         
        {82, "M82", "Cigar Galaxy", MessierObjectType::GALAXY, Constellation::URSA_MAJOR,
         createSkyPosition(9.931231, 69.679703, "M82", "Cigar Galaxy - starburst galaxy"),
         8.4f, 12.0f, QSizeF(11.2, 4.3), "A starburst galaxy with intense star formation", "Spring",
         imaged_objects.contains("M82")},
         
        {101, "M101", "Pinwheel Galaxy", MessierObjectType::GALAXY, Constellation::URSA_MAJOR,
         createSkyPosition(14.053495, 54.348750, "M101", "Pinwheel Galaxy - face-on spiral"),
         7.9f, 27.0f, QSizeF(28.8, 26.9), "A face-on spiral galaxy with prominent arms", "Spring",
         imaged_objects.contains("M101")},
         
        {104, "M104", "Sombrero Galaxy", MessierObjectType::GALAXY, Constellation::VIRGO,
         createSkyPosition(12.666508, -11.623052, "M104", "Sombrero Galaxy with dust lane"),
         8.0f, 29.3f, QSizeF(8.7, 3.5), "A galaxy with a distinctive dust lane like a sombrero", "Spring",
         imaged_objects.contains("M104")},
         
        {106, "M106", "", MessierObjectType::GALAXY, Constellation::CANES_VENATICI,
         createSkyPosition(12.316006, 47.303719, "M106", "Spiral galaxy with active nucleus"),
         8.4f, 22.8f, QSizeF(18.6, 7.6), "A spiral galaxy with an active galactic nucleus", "Spring",
         imaged_objects.contains("M106")},
         
        {109, "M109", "", MessierObjectType::GALAXY, Constellation::URSA_MAJOR,
         createSkyPosition(11.959990, 53.374724, "M109", "Barred spiral galaxy in Ursa Major"),
         20.0f, 55.0f, QSizeF(7.6, 4.7), "A barred spiral galaxy in Ursa Major", "Spring",
         imaged_objects.contains("M109")}
    };
}

SkyPosition MessierCatalog::createSkyPosition(double ra_hours, double dec_degrees, 
                                            const QString& name, const QString& description) {
    SkyPosition pos;
    pos.ra_deg = raHoursToDegrees(ra_hours);  // Convert RA hours to degrees
    pos.dec_deg = dec_degrees;
    pos.name = name;
    pos.description = description;
    return pos;
}

QList<MessierObject> MessierCatalog::getAllObjects() {
    if (m_catalog.isEmpty()) {
        initializeCatalog();
    }
    return m_catalog;
}

MessierObject MessierCatalog::getObjectById(int id) {
    auto objects = getAllObjects();
    for (const auto& obj : objects) {
        if (obj.id == id) {
            return obj;
        }
    }
    // Return empty object if not found
    return MessierObject{};
}

QList<MessierObject> MessierCatalog::getImagedObjects() {
    auto objects = getAllObjects();
    QList<MessierObject> imaged;
    for (const auto& obj : objects) {
        if (obj.has_been_imaged) {
            imaged.append(obj);
        }
    }
    return imaged;
}

QStringList MessierCatalog::getObjectNames() {
    auto objects = getAllObjects();
    QStringList names;
    for (const auto& obj : objects) {
        QString displayName = obj.name;
        if (!obj.common_name.isEmpty()) {
            displayName += " (" + obj.common_name + ")";
        }
        names.append(displayName);
    }
    return names;
}

QString MessierCatalog::objectTypeToString(MessierObjectType type) {
    switch(type) {
        case MessierObjectType::GLOBULAR_CLUSTER: return "Globular Cluster";
        case MessierObjectType::OPEN_CLUSTER: return "Open Cluster";
        case MessierObjectType::NEBULA: return "Nebula";
        case MessierObjectType::PLANETARY_NEBULA: return "Planetary Nebula";
        case MessierObjectType::SUPERNOVA_REMNANT: return "Supernova Remnant";
        case MessierObjectType::GALAXY: return "Galaxy";
        case MessierObjectType::GALAXY_CLUSTER: return "Galaxy Cluster";
        case MessierObjectType::DOUBLE_STAR: return "Double Star";
        case MessierObjectType::ASTERISM: return "Asterism";
        case MessierObjectType::STAR_CLOUD: return "Star Cloud";
        case MessierObjectType::OTHER: return "Other";
        default: return "Unknown";
    }
}

#endif // MESSIERCATALOG_H