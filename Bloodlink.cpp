#include <algorithm>
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
constexpr int BLOOD_EXPIRY_DAYS = 42;
constexpr int URGENCY_PRIORITY_MULTIPLIER = 50;
constexpr int WAITING_DAY_POINTS = 3;

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

// Checks whether donated blood is compatible with the requested blood type.
bool isBloodCompatible(const string& donorType, const string& recipientType) {
    if (donorType == "O-") {
        return true;
    }

    if (donorType == "O+") {
        return recipientType == "O+" || recipientType == "A+" ||
               recipientType == "B+" || recipientType == "AB+";
    }

    if (donorType == "A-") {
        return recipientType == "A-" || recipientType == "A+" ||
               recipientType == "AB-" || recipientType == "AB+";
    }

    if (donorType == "A+") {
        return recipientType == "A+" || recipientType == "AB+";
    }

    if (donorType == "B-") {
        return recipientType == "B-" || recipientType == "B+" ||
               recipientType == "AB-" || recipientType == "AB+";
    }

    if (donorType == "B+") {
        return recipientType == "B+" || recipientType == "AB+";
    }

    if (donorType == "AB-") {
        return recipientType == "AB-" || recipientType == "AB+";
    }

    if (donorType == "AB+") {
        return recipientType == "AB+";
    }

    return false;
}

// Returns the urgency weight used in the priority formula.
int getUrgencyWeight(const string& urgency) {
    if (urgency == "critical") {
        return 2;
    }

    if (urgency == "urgent") {
        return 1;
    }

    return 0;
}

// Calculates the priority score of a request.
int calculatePriorityScore(const BloodRequest& request, int currentDay) {
    int waitingDays = currentDay - request.submitDay;

    return getUrgencyWeight(request.urgency) * URGENCY_PRIORITY_MULTIPLIER +
           waitingDays * WAITING_DAY_POINTS;
}

// Checks whether at least one compatible unit is currently in stock.
bool hasCompatibleUnit(const BloodLinkSystem& system,
                       const BloodRequest& request) {
    for (const BloodUnit& unit : system.units) {
        if (unit.status == UnitStatus::InStock &&
            isBloodCompatible(unit.bloodType, request.bloodType)) {
            return true;
        }
    }

    return false;
}

// Finds the highest-priority request that can currently receive blood.
int findHighestPriorityRequestIndex(const BloodLinkSystem& system) {
    int bestIndex = -1;
    int bestScore = 0;

    for (int i = 0; i < static_cast<int>(system.pendingRequests.size()); i++) {
        const BloodRequest& request = system.pendingRequests[i];

        if (!hasCompatibleUnit(system, request)) {
            continue;
        }

        int score = calculatePriorityScore(request, system.currentDay);

        if (bestIndex == -1 || score > bestScore ||
            (score == bestScore && request.id < system.pendingRequests[bestIndex].id)) {
            bestIndex = i;
            bestScore = score;
        }
    }

    return bestIndex;
}

// Returns the fixed blood value score of a blood type.
int getBloodValue(const string& bloodType) {
    if (bloodType == "AB+") {
        return 1;
    }

    if (bloodType == "AB-") {
        return 2;
    }

    if (bloodType == "A+" || bloodType == "B+") {
        return 3;
    }

    if (bloodType == "A-" || bloodType == "B-") {
        return 4;
    }

    if (bloodType == "O+") {
        return 5;
    }

    return 6;
}

// Checks whether one compatible unit should be used before another unit.
bool isBetterUnit(const BloodUnit& candidate, const BloodUnit& current,
                  int currentDay) {
    int candidateValue = getBloodValue(candidate.bloodType);
    int currentValue = getBloodValue(current.bloodType);

    if (candidateValue != currentValue) {
        return candidateValue < currentValue;
    }

    int candidateShelfLife = BLOOD_EXPIRY_DAYS -
                             (currentDay - candidate.donationDay);
    int currentShelfLife = BLOOD_EXPIRY_DAYS -
                           (currentDay - current.donationDay);

    if (candidateShelfLife != currentShelfLife) {
        return candidateShelfLife < currentShelfLife;
    }

    return candidate.id < current.id;
}

// Finds the best compatible unit that is still in stock.
int findBestCompatibleUnitIndex(const BloodLinkSystem& system,
                                const BloodRequest& request) {
    int bestIndex = -1;

    for (int i = 0; i < static_cast<int>(system.units.size()); i++) {
        const BloodUnit& unit = system.units[i];

        if (unit.status != UnitStatus::InStock ||
            !isBloodCompatible(unit.bloodType, request.bloodType)) {
            continue;
        }

        if (bestIndex == -1 ||
            isBetterUnit(unit, system.units[bestIndex], system.currentDay)) {
            bestIndex = i;
        }
    }

    return bestIndex;
}

// Prints a list of unit IDs separated by single spaces.
void printUnitIds(const vector<int>& unitIds) {
    for (int i = 0; i < static_cast<int>(unitIds.size()); i++) {
        if (i > 0) {
            cout << ' ';
        }

        cout << unitIds[i];
    }
}

// Fulfills at most one eligible request, either fully or partially.
void fulfillRequest(BloodLinkSystem& system) {
    int requestIndex = findHighestPriorityRequestIndex(system);

    if (requestIndex == -1) {
        cout << "No request could be fulfilled\n";
        return;
    }

    BloodRequest& request = system.pendingRequests[requestIndex];
    int remainingBeforeFulfillment = request.remainingQuantity;
    vector<int> usedUnitIds;

    while (request.remainingQuantity > 0) {
        int unitIndex = findBestCompatibleUnitIndex(system, request);

        if (unitIndex == -1) {
            break;
        }

        BloodUnit& unit = system.units[unitIndex];
        unit.status = UnitStatus::Sent;
        usedUnitIds.push_back(unit.id);
        request.remainingQuantity--;
    }

    sort(usedUnitIds.begin(), usedUnitIds.end());

    if (request.remainingQuantity == 0) {
        int hospitalIndex = findHospitalIndex(system, request.hospitalId);

        if (hospitalIndex != -1) {
            system.hospitals[hospitalIndex].fulfilledRequests++;
        }

        cout << "Request " << request.id
             << " fully fulfilled with unit(s): ";
        printUnitIds(usedUnitIds);
        cout << '\n';

        system.pendingRequests.erase(system.pendingRequests.begin() + requestIndex);
    } else {
        cout << "Request " << request.id << " partially fulfilled ("
             << usedUnitIds.size() << '/' << remainingBeforeFulfillment
             << ") with unit(s): ";
        printUnitIds(usedUnitIds);
        cout << '\n';
    }
}

// Prints the IDs belonging to one expiration category.
void printExpirationList(const string& category, const vector<int>& unitIds) {
    if (unitIds.empty()) {
        cout << "No units " << category << '\n';
        return;
    }

    cout << unitIds.size() << " unit(s) " << category << ": ";
    printUnitIds(unitIds);
    cout << '\n';
}

// Advances the system day and updates the expiration status of stocked units.
void advanceDay(BloodLinkSystem& system, int numberOfDays) {
    system.currentDay += numberOfDays;

    vector<int> expiredUnitIds;
    vector<int> unitsToBeExpiredIds;

    for (BloodUnit& unit : system.units) {
        if (unit.status != UnitStatus::InStock) {
            continue;
        }

        int remainingShelfLife = BLOOD_EXPIRY_DAYS -
                                 (system.currentDay - unit.donationDay);

        if (remainingShelfLife <= 0) {
            unit.status = UnitStatus::Expired;
            expiredUnitIds.push_back(unit.id);
        } else if (remainingShelfLife == 1) {
            unitsToBeExpiredIds.push_back(unit.id);
        }
    }

    sort(expiredUnitIds.begin(), expiredUnitIds.end());
    sort(unitsToBeExpiredIds.begin(), unitsToBeExpiredIds.end());

    cout << "Day advanced to " << system.currentDay << '\n';
    printExpirationList("expired", expiredUnitIds);
    printExpirationList("to be expired", unitsToBeExpiredIds);
}

// Reads the donor ID and handles a donation command.
void handleDonateCommand(BloodLinkSystem& system, stringstream& commandStream) {
    int donorId;

    if (commandStream >> donorId) {
        donateBlood(system, donorId);
    }
}

// Reads the request details and handles a request command.
void handleRequestCommand(BloodLinkSystem& system, stringstream& commandStream) {
    int hospitalId;
    string bloodType;
    int quantity;
    string urgency;

    if (commandStream >> hospitalId >> bloodType >> quantity >> urgency) {
        requestBlood(system, hospitalId, bloodType, quantity, urgency);
    }
}

// Handles the command that fulfills one request.
void handleFulfillCommand(BloodLinkSystem& system) {
    fulfillRequest(system);
}

// Reads the number of days and handles an advance_day command.
void handleAdvanceDayCommand(BloodLinkSystem& system,
                             stringstream& commandStream) {
    int numberOfDays;

    if (commandStream >> numberOfDays) {
        advanceDay(system, numberOfDays);
    }
}

// Processes one command entered by the user.
void processCommand(const string& commandLine, BloodLinkSystem& system) {
    stringstream commandStream(commandLine);
    string command;

    commandStream >> command;

    if (command == "donate_blood") {
        handleDonateCommand(system, commandStream);
    } else if (command == "request_blood") {
        handleRequestCommand(system, commandStream);
    } else if (command == "fulfill_request") {
        handleFulfillCommand(system);
    } else if (command == "advance_day") {
        handleAdvanceDayCommand(system, commandStream);
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
