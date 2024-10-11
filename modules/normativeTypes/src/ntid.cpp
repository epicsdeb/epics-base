/* ntid.cpp */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

#include <pv/ntid.h>
#include <pv/typeCast.h>

namespace epics { 

namespace nt {

    const static std::string BAD_NAME = "?"; 

    NTID::NTID(const std::string & id)
    : fullName(id),
      qualifiedName(BAD_NAME),
      namespaceStr(BAD_NAME),
      name(BAD_NAME),
      version(BAD_NAME),
      nsSepIndex(std::string::npos),
      versionSepIndex(std::string::npos),
      nsQualified(false),
      hasVersion(false),
      endMajorIndex(0),
      majorVersionStr(BAD_NAME),
      majorVersionParsed(false),
      hasMajor(false),
      majorVersion(0),

      endMinorIndex(0),
      minorVersionStr(BAD_NAME),
      minorVersionParsed(false),
      hasMinor(false),
      minorVersion(0)
    {
        nsSepIndex = id.find('/');
        nsQualified = nsSepIndex != std::string::npos;
        size_t startIndex = nsQualified ? nsSepIndex+1 : 0;
        versionSepIndex = id.find(':', startIndex);
        hasVersion = versionSepIndex != std::string::npos;
    }

    std::string NTID::getFullName() { return fullName; }

    std::string NTID::getQualifiedName()
    {
        if (qualifiedName == BAD_NAME)
        {
            qualifiedName = hasVersion ?
                fullName.substr(0, versionSepIndex) : fullName;
        }
        return qualifiedName;
    }


    std::string NTID::getNamespace()
    {
        if (namespaceStr == BAD_NAME)
        {
            namespaceStr = nsQualified ?
               fullName.substr(0, nsSepIndex) : "";
        }
        return namespaceStr;
    }

    std::string NTID::getName()
    {
        if (name == BAD_NAME)
        {
            if (hasVersion)
            {
                size_t startIndex = nsQualified ? nsSepIndex+1 : 0;
                name = fullName.substr(startIndex, versionSepIndex);
            }
            else if (nsQualified)
            {
                name = fullName.substr(nsSepIndex+1);
            }
            else
            {
                name = fullName;
            } 
        }
        return name;
    }


    std::string NTID::getVersion()
    {
        if (version == BAD_NAME)
        {
            version = (hasVersion) ? fullName.substr(versionSepIndex+1) : "";
        }
        return version;
    }


    std::string NTID::getMajorVersionString()
    {
        if (majorVersionStr == BAD_NAME)
        {
            if (hasVersion)
            {
                endMajorIndex = fullName.find('.', versionSepIndex+1);
                majorVersionStr = (endMajorIndex != std::string::npos)
                    ? fullName.substr(versionSepIndex+1, endMajorIndex-(versionSepIndex+1)) : 
                      fullName.substr(versionSepIndex+1);
            }
            else
                majorVersionStr = "";
        }
        return majorVersionStr;
    }


    bool NTID::hasMajorVersion()
    {
        if (hasVersion && !majorVersionParsed)
        {
            try {
                using pvData::detail::parseToPOD;
                uint32_t mv;
                parseToPOD(getMajorVersionString(), &mv);
                majorVersion = static_cast<int>(mv);
                hasMajor = true;
            } catch (...) {}
            majorVersionParsed = true;
        }
        return hasMajor;
    }


    int NTID::getMajorVersion()
    {
        // call hasMajorVersion() to calculate values
        hasMajorVersion();
        return majorVersion;
    }


    std::string NTID::getMinorVersionString()
    {
        // call hasMinorVersion() to calculate start of minor
        getMajorVersionString();
        if (minorVersionStr == BAD_NAME)
        {
            if (hasVersion && endMajorIndex != std::string::npos)
            {
                endMinorIndex = fullName.find('.', endMajorIndex+1);
                minorVersionStr = (endMinorIndex != std::string::npos)
                    ? fullName.substr(endMajorIndex+1, endMinorIndex-(endMajorIndex+1)) : 
                      fullName.substr(endMajorIndex+1);
            }
            else
                minorVersionStr = "";
        }
        return minorVersionStr;
    }


    bool NTID::hasMinorVersion()
    {
        if (hasVersion && !minorVersionParsed)
        {
            try {
                using pvData::detail::parseToPOD;
                uint32_t mv;
                parseToPOD(getMinorVersionString(), &mv);
                minorVersion = static_cast<int>(mv);
                hasMinor = true;
            } catch (...) {}
            minorVersionParsed = true;
        }
        return hasMinor;
    }


    int NTID::getMinorVersion()
    {
        // call hasMinorVersion() to calculate values
        hasMinorVersion();
        return minorVersion;
    }


}}


