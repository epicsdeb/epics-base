/* ntid.h */
/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */
#ifndef NTID_H
#define NTID_H

#include <string>

namespace epics { 

namespace nt {

/**
 * @brief Utility class for parsing a type ID that follows the NT type ID conventions
 *
 * An NT type ID will be of the from
@code
    epics:nt/<type-name>:<Major>.<Minor>
@endcode
 * e.g. 
@code
    epics:nt/NTNDArray:1.2
@endcode
 * @author dgh
 */
class NTID
{
public:
    /**
     * Creates an NTID from the specified type ID.
     *
     * @param id the the ID to be parsed.
     * @return NTNDArray instance on success, null otherwise.
     */
    NTID(const std::string &id);
    /**
     * Returns the full name of the id, i.e. the original ID
     * <p>
     * For example above returns "epics:nt/NTNDArray:1.2".
     * @return the full name
     */
    std::string getFullName();

    /**
     * Returns the fully qualified name including namespaces, but excluding version numbers.
     * <p>
     * For example above return "epics:nt/NTNDArray"
     * @return the fully qualified name
     */ 
    std::string getQualifiedName();

    /**
     * Returns the namespace
     * <p>
     * For example above return "epics:nt".
     * @return the namespace
     */
    std::string getNamespace();

    /**
     * Returns the unqualified name, without namespace or version.
     * <p>
     * For example above return "NTNDArray".
     * @return the unqualified name
     */
    std::string getName();

    /**
     * Returns the version as a string.
     * <p>
     * For example above return "NTNDArray".
     * @return the the version string
     */
    std::string getVersion();

    /**
     * Returns the Major version as a string.
     * <p>
     * For example above return "1".
     * @return the Major string
     */
    std::string getMajorVersionString();

    /**
     * Does the ID contain a major version and is it a number.
     * <p>
     * @return true if it contains a major version number
     */
    bool hasMajorVersion();

    /**
     * Returns the Major version as an integer.
     * <p>
     * For example above return 1.
     * @return the Major string
     */
    int getMajorVersion();

    /**
     * Returns the Major version as a string.
     * <p>
     * For example above return "1".
     * @return the Major string
     */
    std::string getMinorVersionString();

    /**
     * Does the ID contain a minor version and is it a number.
     * <p>
     * @return true if it contains a minor version number
     */
    bool hasMinorVersion();

    /**
     * Returns the Minor version as an integer.
     * <p>
     * For example above return 1.
     * @return the Minor string
     */
    int getMinorVersion();

private:
    std::string fullName;
    std::string qualifiedName;
    std::string namespaceStr;
    std::string name;
    std::string version;

    size_t nsSepIndex;
    size_t versionSepIndex;
    bool nsQualified;
    bool hasVersion;

    size_t endMajorIndex;
    std::string majorVersionStr;
    bool majorVersionParsed;
    bool hasMajor;
    int majorVersion;

    size_t endMinorIndex;
    std::string minorVersionStr;
    bool minorVersionParsed;
    bool hasMinor;
    int minorVersion;

};

}}

#endif

