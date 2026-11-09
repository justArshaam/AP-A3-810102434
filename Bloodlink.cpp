#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Constants used when the system starts.
constexpr int FIRST_DAY = 1;
constexpr int FIRST_ID = 1;
constexpr int NO_DONATION_DAY = 0;

// Stores information about one hospital.
struct Hospital {
    int id = 0;
    string name;
    int totalRequests = 0;
    int fulfilledRequests = 0;
};

// Stores information about one registered donor.
struct Donor {
    int id = 0;
    string name;
    string bloodType;
    bool hasDonated = false;
    int lastDonationDay = NO_DONATION_DAY;
};

enum class UnitStatus {
    InStock,
    Sent,
    Expired
};

// Stores information about one donated blood unit.
struct BloodUnit {
    int id = 0;
    int donorId = 0;
    string bloodType;
    int donationDay = 0;
    UnitStatus status = UnitStatus::InStock;
};

// Stores information about one hospital blood request.
struct BloodRequest {
    int id = 0;
    int hospitalId = 0;
    string bloodType;
    int quantity = 0;
    int remainingQuantity = 0;
    string urgency;
    int submitDay = 0;
};

// Holds all data and counters belonging to the BloodLink system.
struct BloodLinkSystem {
    vector<Hospital> hospitals;
    vector<Donor> donors;
    vector<BloodUnit> units;
    vector<BloodRequest> pendingRequests;

    int currentDay = FIRST_DAY;
    int nextUnitId = FIRST_ID;
    int nextRequestId = FIRST_ID;
};

// Removes the carriage return that may appear at the end of a CSV line.
void removeCarriageReturn(string& value) {
    if (!value.empty() && value.back() == '\r') {
        value.pop_back();
    }
}

// Reads hospital information from a CSV file.
bool loadHospitals(const string& fileName, BloodLinkSystem& system) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cerr << "Error: could not open hospitals file.\n";
        return false;
    }

    string line;
    getline(file, line); // Skip the header row.

    while (getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        stringstream lineStream(line);
        string idText;
        string name;

        getline(lineStream, idText, ',');
        getline(lineStream, name, ',');
        removeCarriageReturn(name);

        Hospital hospital;
        hospital.id = stoi(idText);
        hospital.name = name;
        system.hospitals.push_back(hospital);
    }

    return true;
}

// Reads donor information from a CSV file.
bool loadDonors(const string& fileName, BloodLinkSystem& system) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cerr << "Error: could not open donors file.\n";
        return false;
    }

    string line;
    getline(file, line); // Skip the header row.

    while (getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        stringstream lineStream(line);
        string idText;
        string name;
        string bloodType;

        getline(lineStream, idText, ',');
        getline(lineStream, name, ',');
        getline(lineStream, bloodType, ',');
        removeCarriageReturn(bloodType);

        Donor donor;
        donor.id = stoi(idText);
        donor.name = name;
        donor.bloodType = bloodType;
        system.donors.push_back(donor);
    }

    return true;
}

// Loads both input files in the required order.
bool loadInitialData(int argc, char* argv[], BloodLinkSystem& system) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <hospitals.csv> <donors.csv>\n";
        return false;
    }

    if (!loadHospitals(argv[1], system)) {
        return false;
    }

    if (!loadDonors(argv[2], system)) {
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    BloodLinkSystem system;

    if (!loadInitialData(argc, argv, system)) {
        return 1;
    }

    return 0;
}
