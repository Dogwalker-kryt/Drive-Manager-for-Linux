# Drive Benchmarking System Documentation  
Includes: **Sequential Write**, **Sequential Read**, **Random Read (IOPS)**, **Classification**, **Result Printing**, **Benchmark_main()**

---

## `benchmarkSequentialWrite()`

```cpp
double benchmarkSequentialWrite(const std::string& path, size_t totalMB = 512) {
    const size_t blockSize = 4 * 1024 * 1024; // 4MB blocks
    std::vector<char> buffer(blockSize, 'A');

    std::string tempFile = path + "/dmgr_benchmark.tmp";
    std::ofstream out(tempFile, std::ios::binary);

    if (!out.is_open()) return -1;

    auto start = std::chrono::high_resolution_clock::now();

    size_t written = 0;
    while (written < totalMB * 1024 * 1024) {
        out.write(buffer.data(), blockSize);
        written += blockSize;
    }

    out.close();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();
    double mbps = (double)totalMB / seconds;

    std::remove(tempFile.c_str());
    return mbps;
}
```

### Overview
Performs a **sequential write benchmark** by writing a temporary file in large 4MB blocks until `totalMB` is reached.

### Behavior
- Writes `totalMB` megabytes to a temp file.
- Measures elapsed time.
- Returns throughput in **MB/s**.
- Deletes the temp file afterward.
- Returns `-1` if the file cannot be created.

---

## `benchmarkSequentialRead()`

```cpp
double benchmarkSequentialRead(const std::string& path, size_t totalMB = 512) {
    const size_t blockSize = 4 * 1024 * 1024;

    std::string tempFile = path + "/dmgr_benchmark.tmp";
    {
        std::ofstream out(tempFile, std::ios::binary);
        std::vector<char> buffer(blockSize, 'A');
        for (size_t i = 0; i < totalMB * 1024 * 1024; i += blockSize)
            out.write(buffer.data(), blockSize);
    }

    std::ifstream in(tempFile, std::ios::binary);
    if (!in.is_open()) return -1;

    std::vector<char> buffer(blockSize);

    auto start = std::chrono::high_resolution_clock::now();

    while (in.read(buffer.data(), blockSize)) {}

    auto end = std::chrono::high_resolution_clock::now();
    in.close();

    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();
    double mbps = (double)totalMB / seconds;

    std::remove(tempFile.c_str());
    return mbps;
}
```

### Overview
Performs a **sequential read benchmark** by reading a pre‑written file in 4MB blocks.

### Behavior
- Creates a temp file of size `totalMB`.
- Reads it sequentially.
- Measures time and returns **MB/s**.
- Deletes the file afterward.

---

## `benchmarkRandomRead()`

```cpp
double benchmarkRandomRead(const std::string& path, size_t operations = 50000) {
    const size_t blockSize = 4096;

    std::string tempFile = path + "/dmgr_benchmark.tmp";
    {
        std::ofstream out(tempFile, std::ios::binary);
        std::vector<char> buffer(blockSize, 'A');
        for (size_t i = 0; i < operations * blockSize; i += blockSize)
            out.write(buffer.data(), blockSize);
    }

    std::ifstream in(tempFile, std::ios::binary);
    if (!in.is_open()) return -1;

    std::vector<char> buffer(blockSize);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < operations; i++) {
        size_t offset = (rand() % operations) * blockSize;
        in.seekg(offset);
        in.read(buffer.data(), blockSize);
    }

    auto end = std::chrono::high_resolution_clock::now();
    in.close();

    std::chrono::duration<double> diff = end - start;
    double seconds = diff.count();

    std::remove(tempFile.c_str());
    return operations / seconds; // IOPS
}
```

### Overview
Performs a **random read benchmark** using 4KB blocks.

### Behavior
- Creates a temp file sized `operations * 4KB`.
- Performs `operations` random reads.
- Measures time and returns **IOPS** (operations per second).
- Deletes the file afterward.

---

## Drive Classification System

### `Rule` struct

```cpp
struct Rule {
    std::function<bool(double,double,double)> match;
    std::string label;
};
```

Defines a rule consisting of:
- a lambda that checks performance thresholds  
- a label describing the drive type  

---

## `classifyDrive()`

```cpp
std::string classifyDrive(double w, double r, double iops) {
    std::vector<Rule> rules = {
        { [](double w, double r, double i){ return w > 0 && r > 0; },
          "The Benchmark failed\n" },

        { [](double w, double r, double i){ return w < 50 && r < 200; },
          "Your device has the performance of a USB Flash Drive\n" },

        { [](double w, double r, double i){ return w < 150 && r < 150; },
          "Your device has the performance of a Hard Disk Drive (HDD)\n" },

        { [](double w, double r, double i){ return w < 600 && r < 600; },
          "Your device has the performance of a SATA SSD\n" },

        { [](double w, double r, double i){ return w < 3500 && r < 3500; },
          "Your device has the performance of an NVMe SSD (Gen3)\n" },

        { [](double w, double r, double i){ return w >= 3500; },
          "Your device has the performance of a high‑performance NVMe SSD (Gen4/Gen5)\n" }
    };

    for (auto& rule : rules)
        if (rule.match(w, r, iops))
            return rule.label;

    return "[ERROR] Program failed to classify your Drive type\nDrive type: Unkown\n";
}
```

### Overview
Classifies the drive based on:
- sequential write speed  
- sequential read speed  
- random read IOPS  

Uses a rule‑based system to determine approximate drive class.

---

## `printBenchmarkSpeeds()`

```cpp
void printBenchmarkSpeeds(double wirte_speed_seq, double read_speed_seq, double iops_speed) {
    ...
    std::cout << BOLD << "\n[Benchmark results]\n" << RESET;
    std::cout << " Sequential Write speed : " << w_str2 << " MB/s\n";
    std::cout << " Sequential Read speed  : " << r_str2 << " MB/s\n";
    std::cout << " IOPS speed             : " << i_str2 << " IOPS\n";

    std::string aprox_drive_type = classifyDrive(wirte_speed_seq, read_speed_seq, iops_speed);
    std::cout << aprox_drive_type;
}
```

### Overview
Formats and prints:
- sequential write speed  
- sequential read speed  
- random read IOPS  
- drive classification  

---

## `Benchmark_main()`

```cpp
void Benchmark_main() {
    std::cout << BOLD << "\n[Drive Benchmark Tool]\n" << RESET;
    std::cout << "Enter a path like '/home', '/mnt/drive' or 'media/[user]/usb_device' to benchmark\n";
    std::string benchmark_drive_name;
    std::cout << "Make sure you have around 1 GB free space on the Drive\n";
    std::cout << "And dont kill the program during the Benchmark, this can cause the temp benchmark file to be not deleted!";
    std::cin >> benchmark_drive_name;

    double write_speed_seq = benchmarkSequentialWrite(benchmark_drive_name);
    double read_speed_seq = benchmarkSequentialRead(benchmark_drive_name);
    double iops_speed = benchmarkRandomRead(benchmark_drive_name);

    printBenchmarkSpeeds(write_speed_seq, read_speed_seq, iops_speed);
}
```

### Overview
Main entry point for the benchmarking tool.

### Behavior
1. Prompts user for a directory path.
2. Warns about required free space.
3. Runs:
   - sequential write benchmark  
   - sequential read benchmark  
   - random read benchmark  
4. Prints results and classification.

---
