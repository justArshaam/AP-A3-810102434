#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Constants used by the system.
constexpr int FIRST_DAY = 1;
constexpr int FIRST_UNIT_ID = 1;
constexpr int FIRST_REQUEST_ID = 1;
constexpr int NO_DONATION_DAY = 0;
constexpr int DONATION_INTERVAL_DAYS = 56;

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
    int nextUnitId = FIRST_UNIT_ID;
    int nextRequestId = FIRST_REQUEST_ID;
};

// Removes the carriage return that may appear at the end of a CSV line.
void removeCarriageReturn(string& value) {
    if (!value.empty() && value.back() == '\r') {
        value.pop_back();
    }
}

// Finds a hospital by its ID and returns its index.
int findHospitalIndex(const BloodLinkSystem& system, int hospitalId) {
    int index = 0;

    for (const Hospital& hospital : system.hospitals) {
        if (hospital.id == hospitalId) {
            return index;
        }

        index++;
    }

    return -1;
}

// Finds a donor by its ID and returns its index.
int findDonorIndex(const BloodLinkSystem& system, int donorId) {
    int index = 0;

    for (const Donor& donor : system.donors) {
        if (donor.id == donorId) {
            return index;
        }

        index++;
    }

    return -1;
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

// Registers a blood donation if the donor is eligible.
void donateBlood(BloodLinkSystem& system, int donorId) {
    int donorIndex = findDonorIndex(system, donorId);

    if (donorIndex == -1) {
        return;
    }

    Donor& donor = system.donors[donorIndex];

    if (donor.hasDonated) {
        int daysSinceLastDonation = system.currentDay - donor.lastDonationDay;

        if (daysSinceLastDonation < DONATION_INTERVAL_DAYS) {
            int daysRemaining = DONATION_INTERVAL_DAYS - daysSinceLastDonation;

            cout << "Donor " << donor.id << " (" << donor.name
                 << ") not eligible, " << daysRemaining
                 << " days remaining\n";
            return;
        }
    }

    BloodUnit unit;
    unit.id = system.nextUnitId;
    unit.donorId = donor.id;
    unit.bloodType = donor.bloodType;
    unit.donationDay = system.currentDay;
    unit.status = UnitStatus::InStock;

    system.units.push_back(unit);
    system.nextUnitId++;

    donor.hasDonated = true;
    donor.lastDonationDay = system.currentDay;

    cout << "Unit " << unit.id << " donated by donor " << donor.id
         << " (" << donor.name << "), type " << donor.bloodType << '\n';
}

// Registers a new blood request for a hospital.
void requestBlood(BloodLinkSystem& system, int hospitalId,
                  const string& bloodType, int quantity,
                  const string& urgency) {
    int hospitalIndex = findHospitalIndex(system, hospitalId);

    if (hospitalIndex == -1) {
        return;
    }

    BloodRequest request;
    request.id = system.nextRequestId;
    request.hospitalId = hospitalId;
    request.bloodType = bloodType;
    request.quantity = quantity;
    request.remainingQuantity = quantity;
    request.urgency = urgency;
    request.submitDay = system.currentDay;

    system.pendingRequests.push_back(request);
    system.nextRequestId++;
    system.hospitals[hospitalIndex].totalRequests++;

    cout << "Request " << request.id << " registered\n";
}

// Processes one command entered by the user.
void processCommand(const string& commandLine, BloodLinkSystem& system) {
    stringstream commandStream(commandLine);
    string command;

    commandStream >> command;

    if (command == "donate_blood") {
        int donorId;

        if (commandStream >> donorId) {
            donateBlood(system, donorId);
        }
    } else if (command == "request_blood") {
        int hospitalId;
        string bloodType;
        int quantity;
        string urgency;

        if (commandStream >> hospitalId >> bloodType >> quantity >> urgency) {
            requestBlood(system, hospitalId, bloodType, quantity, urgency);
        }
    }
}

// Reads and processes commands until the input ends.
void processCommands(BloodLinkSystem& system) {
    string commandLine;

    while (getline(cin, commandLine)) {
        if (!commandLine.empty()) {
            processCommand(commandLine, system);
        }
    }
}

int main(int argc, char* argv[]) {
    BloodLinkSystem system;

    if (!loadInitialData(argc, argv, system)) {
        return 1;
    }

    processCommands(system);

    return 0;
}
