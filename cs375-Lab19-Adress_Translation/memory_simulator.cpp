#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <limits>

enum Protection { READ_ONLY, READ_WRITE };
enum Policy { FIFO, LRU };

const int MAX_OVERCOMMIT = 4;

struct Page {
    int frame_number = -1;  // -1 if not in memory
    bool present = false;
    Protection protection = READ_WRITE;
    int last_access = 0;  // For LRU
};

struct Segment {
    int base_address;
    int limit;  // Number of pages
    Protection protection;
};

struct FrameOwner {
    int seg = -1;
    int dir = -1;
    int page = -1;
};

struct Metrics {
    int translations = 0;
    int successes = 0;
    int failures = 0;
    int pageFaults = 0;
    int replacements = 0;
    long long totalLatency = 0;
};

std::string protName(Protection p) { return p == READ_ONLY ? "RO" : "RW"; }
std::string policyName(Policy p) { return p == FIFO ? "FIFO" : "LRU"; }

class PageTable {
public:
    std::vector<Page> pages;
    int page_size;

    PageTable() : page_size(1000) {}

    PageTable(int numPages, int pageSize) : page_size(pageSize) {
        pages.resize(numPages);
        for (auto& p : pages) {
            p.present = rand() % 2;  // Randomly present
            if (p.present) p.frame_number = rand() % 100;
            p.protection = (rand() % 2) ? READ_WRITE : READ_ONLY;
        }
    }

    static PageTable emptyTable(int numPages, int pageSize) {
        PageTable t;
        t.page_size = pageSize;
        t.pages.resize(numPages);
        return t;
    }

    int getFrameNumber(int pageNum, int time, Protection accessType, std::string& error) {
        if (pageNum < 0 || pageNum >= (int)pages.size()) {
            error = "Page Fault: Invalid page number " + std::to_string(pageNum) +
                    ", max " + std::to_string((int)pages.size() - 1);
            return -3;
        }
        if (accessType == READ_WRITE && pages[pageNum].protection == READ_ONLY) {
            error = "Protection Violation: Cannot write to read-only page " + std::to_string(pageNum);
            return -2;
        }
        if (!pages[pageNum].present) {
            error = "Page Fault: Page " + std::to_string(pageNum) + " not in memory";
            return -1;
        }
        pages[pageNum].last_access = time;
        return pages[pageNum].frame_number;
    }

    void setFrame(int pageNum, int frame, Protection prot) {
        if (pageNum >= 0 && pageNum < (int)pages.size()) {
            pages[pageNum].frame_number = frame;
            pages[pageNum].present = true;
            pages[pageNum].protection = prot;
        }
    }

    void evict(int pageNum) {
        if (pageNum >= 0 && pageNum < (int)pages.size()) {
            pages[pageNum].frame_number = -1;
            pages[pageNum].present = false;
        }
    }
};

class DirectoryTable {
public:
    std::map<int, PageTable> tables;
    int num_pages;
    int pages_per_table;
    int page_size;

    DirectoryTable() : num_pages(0), pages_per_table(1), page_size(1000) {}

    DirectoryTable(int numPages, int pageSize, int pagesPerTable, bool randomInit)
        : num_pages(numPages), pages_per_table(pagesPerTable), page_size(pageSize) {
        if (randomInit) {
            for (int d = 0; d < numDirs(); ++d) {
                if (rand() % 3 != 0) tables[d] = PageTable(pagesIn(d), page_size);
            }
        }
    }

    int numDirs() const { return (num_pages + pages_per_table - 1) / pages_per_table; }

    int pagesIn(int dir) const {
        if (dir < 0 || dir >= numDirs()) return 0;
        return std::min(pages_per_table, num_pages - dir * pages_per_table);
    }

    bool hasTable(int dir) const { return tables.count(dir) > 0; }

    PageTable& getTable(int dir) {
        if (!hasTable(dir)) tables[dir] = PageTable::emptyTable(pagesIn(dir), page_size);
        return tables[dir];
    }

    int entriesUsed() const {
        int total = numDirs();
        for (const auto& t : tables) total += (int)t.second.pages.size();
        return total;
    }
};

class PhysicalMemory {
public:
    int num_frames;
    std::vector<bool> free_frames;
    std::queue<int> fifo_queue;  // For FIFO replacement
    std::vector<int> last_use;
    std::vector<FrameOwner> owner;
    int time = 0;

    PhysicalMemory(int frames) : num_frames(frames) {
        free_frames.resize(frames, true);
        last_use.resize(frames, 0);
        owner.resize(frames);
    }

    int takeFreeFrame() {
        for (int i = 0; i < num_frames; ++i) {
            if (free_frames[i]) {
                free_frames[i] = false;
                fifo_queue.push(i);
                last_use[i] = time;
                return i;
            }
        }
        return -1;
    }

    int allocateFrame(Policy policy, bool& replaced) {
        replaced = false;
        int freeFrame = takeFreeFrame();
        if (freeFrame != -1) return freeFrame;
        replaced = true;
        if (policy == LRU) {
            int victim = 0;
            for (int i = 1; i < num_frames; ++i) {
                if (last_use[i] < last_use[victim]) victim = i;
            }
            last_use[victim] = time;
            return victim;
        }
        // FIFO replacement
        int frame = fifo_queue.front();
        fifo_queue.pop();
        fifo_queue.push(frame);
        last_use[frame] = time;
        return frame;
    }

    void touch(int frame) {
        if (frame >= 0 && frame < num_frames) last_use[frame] = time;
    }

    void freeFrame(int frame) {
        if (frame >= 0 && frame < num_frames) {
            free_frames[frame] = true;
            owner[frame] = FrameOwner();
        }
    }

    int usedFrames() const {
        return (int)std::count(free_frames.begin(), free_frames.end(), false);
    }

    double utilization() const {
        return (double)usedFrames() / num_frames * 100;
    }
};

std::string addrText(int seg, int dir, int page, int offset, Protection access) {
    return "(" + std::to_string(seg) + "," + std::to_string(dir) + "," + std::to_string(page) + "," +
           std::to_string(offset) + "," + (access == READ_ONLY ? "R" : "W") + ")";
}

class SegmentTable {
public:
    std::vector<Segment> segments;
    std::map<int, DirectoryTable> pageTables;
    PhysicalMemory physMem;
    Policy policy;
    int page_size;
    int pages_per_table;
    Metrics metrics;
    std::vector<std::string> timeline;
    std::string lastError;
    bool lastFault = false;
    bool lastReplaced = false;
    std::ofstream errorLog;

    SegmentTable(int numFrames, int pageSize = 1000, int pagesPerTable = 4, Policy pol = FIFO)
        : physMem(numFrames), policy(pol), page_size(pageSize), pages_per_table(pagesPerTable) {
        errorLog.open("errors.txt", std::ios::app);
        errorLog << "---- New run: " << numFrames << " frames, page size " << pageSize
                 << ", policy " << policyName(pol) << " ----\n";
    }

    void addSegment(int id, int base, int limit, Protection prot, bool randomInit = true) {
        segments.push_back({base, limit, prot});
        pageTables[id] = DirectoryTable(limit, page_size, pages_per_table, randomInit);  // Default page size 1000
        DirectoryTable& dt = pageTables[id];
        for (auto& entry : dt.tables) {
            for (size_t p = 0; p < entry.second.pages.size(); ++p) {
                Page& pg = entry.second.pages[p];
                if (!pg.present) continue;
                int frame = physMem.takeFreeFrame();
                if (frame == -1) {
                    pg.present = false;
                    pg.frame_number = -1;
                    continue;
                }
                pg.frame_number = frame;
                physMem.owner[frame] = {id, entry.first, (int)p};
            }
        }
    }

    bool preloadPage(int segNum, int linearPage, Protection prot) {
        DirectoryTable& dt = pageTables[segNum];
        int dir = linearPage / pages_per_table;
        int page = linearPage % pages_per_table;
        PageTable& pt = dt.getTable(dir);
        pt.pages[page].protection = prot;
        if (pt.pages[page].present) return true;
        int frame = physMem.takeFreeFrame();
        if (frame == -1) return false;
        pt.setFrame(page, frame, prot);
        physMem.owner[frame] = {segNum, dir, page};
        return true;
    }

    void evict(int frame) {
        FrameOwner o = physMem.owner[frame];
        if (o.seg < 0) return;
        pageTables[o.seg].tables[o.dir].evict(o.page);
        physMem.owner[frame] = FrameOwner();
    }

    int fail(int now, const std::string& label, const std::string& msg) {
        metrics.failures++;
        lastError = msg;
        errorLog << "Time " << now << ": " << label << " -> " << msg << "\n";
        errorLog.flush();
        timeline.push_back("Time " + std::to_string(now) + ": " + label + " failed - " + msg);
        return -1;
    }

    int translateAddress(int segNum, int pageDir, int pageNum, int offset, Protection accessType, int& latency) {
        latency = 1 + rand() % 5;  // Simulated latency
        int now = physMem.time++;
        metrics.translations++;
        metrics.totalLatency += latency;
        lastError = "";
        lastFault = false;
        lastReplaced = false;
        std::string label = addrText(segNum, pageDir, pageNum, offset, accessType);

        if (segNum < 0 || segNum >= (int)segments.size()) {
            return fail(now, label, "Segmentation Fault: Invalid segment " + std::to_string(segNum) +
                                    ", max " + std::to_string((int)segments.size() - 1));
        }
        Segment segment = segments[segNum];
        DirectoryTable& dt = pageTables[segNum];
        if (pageDir < 0 || pageDir >= dt.numDirs()) {
            return fail(now, label, "Directory Fault: Invalid directory " + std::to_string(pageDir) +
                                    " in segment " + std::to_string(segNum) + ", max " +
                                    std::to_string(dt.numDirs() - 1));
        }
        if (pageNum < 0 || pageNum >= dt.pagesIn(pageDir)) {
            return fail(now, label, "Page Fault: Page " + std::to_string(pageNum) + " exceeds limit " +
                                    std::to_string(dt.pagesIn(pageDir)) + " in directory " +
                                    std::to_string(pageDir) + " of segment " + std::to_string(segNum));
        }
        if (offset < 0 || offset >= page_size) {
            return fail(now, label, "Offset Fault: Offset " + std::to_string(offset) +
                                    " exceeds page size " + std::to_string(page_size));
        }
        if (accessType == READ_WRITE && segment.protection == READ_ONLY) {
            return fail(now, label, "Protection Violation: Cannot write to read-only segment " +
                                    std::to_string(segNum));
        }

        PageTable& pt = dt.getTable(pageDir);
        std::string err;
        int frame = pt.getFrameNumber(pageNum, now, accessType, err);
        if (frame == -2 || frame == -3) {
            return fail(now, label, err + " (segment " + std::to_string(segNum) + ", directory " +
                                    std::to_string(pageDir) + ")");
        }
        if (frame == -1) {
            // Handle page fault: Allocate new frame
            metrics.pageFaults++;
            lastFault = true;
            bool replaced = false;
            frame = physMem.allocateFrame(policy, replaced);
            if (replaced) {
                evict(frame);
                metrics.replacements++;
                lastReplaced = true;
            }
            pt.setFrame(pageNum, frame, pt.pages[pageNum].protection);
            pt.pages[pageNum].last_access = now;
            physMem.owner[frame] = {segNum, pageDir, pageNum};
        }
        physMem.touch(frame);

        int physical = segment.base_address + frame * page_size + offset;
        metrics.successes++;
        std::string note;
        if (lastFault) note = lastReplaced ? " (page fault, replaced frame " + std::to_string(frame) + ")"
                                           : " (page fault, loaded into free frame " + std::to_string(frame) + ")";
        timeline.push_back("Time " + std::to_string(now) + ": " + label + " translated to " +
                           std::to_string(physical) + note);
        return physical;
    }

    void printMemoryMap() {
        std::cout << "\nMemory Map (policy " << policyName(policy) << ", page size " << page_size
                  << ", " << pages_per_table << " pages per directory table):\n";
        int singleLevel = 0;
        int twoLevel = 0;
        for (size_t i = 0; i < segments.size(); ++i) {
            DirectoryTable& dt = pageTables[(int)i];
            std::cout << "Segment " << i << ": Base=" << segments[i].base_address
                      << ", Limit=" << segments[i].limit
                      << ", Protection=" << (segments[i].protection == READ_ONLY ? "RO" : "RW")
                      << ", Directories=" << dt.numDirs() << "\n";
            for (int d = 0; d < dt.numDirs(); ++d) {
                if (!dt.hasTable(d)) {
                    std::cout << "  Dir " << d << ": not allocated (" << dt.pagesIn(d) << " pages, none in memory)\n";
                    continue;
                }
                std::cout << "  Dir " << d << ": page table allocated\n";
                PageTable& pt = dt.tables[d];
                for (size_t j = 0; j < pt.pages.size(); ++j) {
                    auto& p = pt.pages[j];
                    std::cout << "    Page " << j << ": Frame=" << p.frame_number
                              << ", Present=" << p.present
                              << ", Protection=" << (p.protection == READ_ONLY ? "RO" : "RW") << "\n";
                }
            }
            singleLevel += segments[i].limit;
            twoLevel += dt.entriesUsed();
        }

        std::cout << "\nFrame Map:\n";
        for (int f = 0; f < physMem.num_frames; ++f) {
            std::string cell;
            if (physMem.free_frames[f]) {
                cell = "free";
            } else {
                FrameOwner o = physMem.owner[f];
                cell = "S" + std::to_string(o.seg) + ".D" + std::to_string(o.dir) + ".P" + std::to_string(o.page);
            }
            std::cout << "  [F" << std::setw(2) << std::setfill('0') << f << std::setfill(' ') << " "
                      << std::left << std::setw(10) << cell << std::right << "]";
            if ((f + 1) % 4 == 0 || f == physMem.num_frames - 1) std::cout << "\n";
        }
        std::cout << "Page table entries: single-level would use " << singleLevel
                  << ", two-level is using " << twoLevel << "\n";
        std::cout << "Physical Memory Utilization: " << physMem.utilization() << "%\n";
    }

    void printTimeline() const {
        std::cout << "\nTranslation Timeline:\n";
        if (timeline.empty()) std::cout << "  (nothing yet)\n";
        for (const auto& line : timeline) std::cout << "  " << line << "\n";
    }

    void printMetrics() const {
        std::cout << "\nMetrics:\n";
        std::cout << "  Translations: " << metrics.translations << "\n";
        std::cout << "  Successful: " << metrics.successes << "\n";
        std::cout << "  Failed: " << metrics.failures << "\n";
        std::cout << "  Page faults handled: " << metrics.pageFaults << "\n";
        std::cout << "  Replacements (" << policyName(policy) << "): " << metrics.replacements << "\n";
        if (metrics.translations > 0) {
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "  Page fault rate: " << (double)metrics.pageFaults / metrics.translations * 100 << "%\n";
            std::cout << "  Average latency: " << (double)metrics.totalLatency / metrics.translations << " units\n";
            std::cout.unsetf(std::ios::fixed);
            std::cout << std::setprecision(6);
        }
        std::cout << "  Physical Memory Utilization: " << physMem.utilization() << "%\n";
    }
};

void generateRandomAddresses(SegmentTable& st, int num, double validRatio, const std::string& logFile) {
    std::ofstream log(logFile);
    std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
    std::bernoulli_distribution pickValid(validRatio);
    Metrics before = st.metrics;
    int expectedValid = 0;
    long long latencySum = 0;
    int faults = 0;

    log << "Stress test: " << num << " addresses, valid ratio " << validRatio
        << ", policy " << policyName(st.policy) << "\n";
    for (int i = 0; i < num; ++i) {
        int segCount = (int)st.segments.size();
        int segNum = gen() % segCount;
        DirectoryTable& dt = st.pageTables[segNum];
        int pageDir = gen() % dt.numDirs();
        int pageNum = gen() % dt.pagesIn(pageDir);
        int offset = gen() % st.page_size;
        Protection access = (gen() % 2) ? READ_WRITE : READ_ONLY;
        bool valid = pickValid(gen);

        if (valid) {
            expectedValid++;
            bool pageRO = dt.hasTable(pageDir) && dt.tables[pageDir].pages[pageNum].protection == READ_ONLY;
            if (st.segments[segNum].protection == READ_ONLY || pageRO) access = READ_ONLY;
        } else {
            int kind = gen() % 5;
            if (kind == 0) segNum = segCount + gen() % 3;
            else if (kind == 1) pageDir = dt.numDirs() + gen() % 2;
            else if (kind == 2) pageNum = dt.pagesIn(pageDir) + gen() % 3;
            else if (kind == 3) offset = st.page_size + gen() % 100;
            else {
                int roSeg = -1;
                for (int s = 0; s < segCount; ++s) {
                    if (st.segments[s].protection == READ_ONLY) { roSeg = s; break; }
                }
                if (roSeg != -1) {
                    segNum = roSeg;
                    pageDir = 0;
                    pageNum = 0;
                    access = READ_WRITE;
                } else {
                    segNum = segCount + gen() % 3;
                }
            }
        }

        int latency;
        log << "Address " << i << ": " << addrText(segNum, pageDir, pageNum, offset, access)
            << (valid ? " [expected valid] " : " [expected invalid] ");
        int addr = st.translateAddress(segNum, pageDir, pageNum, offset, access, latency);
        latencySum += latency;
        if (addr == -1) {
            faults++;
            log << "Failed: " << st.lastError << "\n";
        } else {
            log << "Physical=" << addr << ", Latency=" << latency
                << (st.lastFault ? (st.lastReplaced ? ", page fault + replacement" : ", page fault") : "") << "\n";
        }
    }

    int pageFaults = st.metrics.pageFaults - before.pageFaults;
    int replacements = st.metrics.replacements - before.replacements;
    log << "\nSummary\n";
    log << "Expected valid: " << expectedValid << ", expected invalid: " << num - expectedValid << "\n";
    log << "Successful translations: " << num - faults << "\n";
    log << "Failed translations: " << faults << " (" << (double)faults / num * 100 << "%)\n";
    log << "Page Fault Rate: " << (double)pageFaults / num * 100 << "%\n";
    log << "Replacements: " << replacements << "\n";
    log << "Average latency: " << (double)latencySum / num << " units\n";
    log << "Physical Memory Utilization: " << st.physMem.utilization() << "%\n";

    std::cout << "Stress test done: " << num - faults << " ok, " << faults << " failed, "
              << pageFaults << " page faults, " << replacements << " replacements\n";
    std::cout << "Physical Memory Utilization: " << st.physMem.utilization() << "%\n";
}

bool parseProt(const std::string& s, Protection& out) {
    if (s == "RO" || s == "ro" || s == "R" || s == "r" || s == "0") { out = READ_ONLY; return true; }
    if (s == "RW" || s == "rw" || s == "W" || s == "w" || s == "1") { out = READ_WRITE; return true; }
    return false;
}

void processBatch(SegmentTable& st, const std::string& fileName) {
    std::ifstream in(fileName);
    if (!in) {
        std::cout << "Could not open " << fileName << "\n";
        return;
    }
    Metrics before = st.metrics;
    std::string line;
    int lineNum = 0;
    int skipped = 0;
    while (std::getline(in, line)) {
        lineNum++;
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream ss(line);
        int seg, dir, page, offset;
        std::string acc;
        if (!(ss >> seg)) continue;
        Protection access;
        if (!(ss >> dir >> page >> offset >> acc) || !parseProt(acc, access)) {
            std::cout << "Line " << lineNum << ": bad format, skipped\n";
            skipped++;
            continue;
        }
        int latency;
        int addr = st.translateAddress(seg, dir, page, offset, access, latency);
        std::cout << addrText(seg, dir, page, offset, access) << " -> ";
        if (addr == -1) std::cout << st.lastError << "\n";
        else std::cout << "Physical Address: " << addr << ", Latency: " << latency
                       << (st.lastFault ? " (page fault handled)" : "") << "\n";
    }
    int total = st.metrics.translations - before.translations;
    std::cout << "\nBatch Summary for " << fileName << ":\n";
    std::cout << "  Addresses processed: " << total << " (" << skipped << " bad lines skipped)\n";
    std::cout << "  Successful translations: " << st.metrics.successes - before.successes << "\n";
    std::cout << "  Failed translations: " << st.metrics.failures - before.failures << "\n";
    std::cout << "  Page faults handled: " << st.metrics.pageFaults - before.pageFaults << "\n";
    std::cout << "  Replacements: " << st.metrics.replacements - before.replacements << "\n";
    if (total > 0) {
        std::cout << "  Average latency: "
                  << (double)(st.metrics.totalLatency - before.totalLatency) / total << " units\n";
    }
    std::cout << "  Physical Memory Utilization: " << st.physMem.utilization() << "%\n";
}

struct SegConfig {
    int id;
    long long base;
    int limit;
    Protection prot;
    bool randomPages;
    std::vector<std::pair<int, Protection>> preload;
};

struct Config {
    int frames = 0;
    int pageSize = 1000;
    int dirSize = 4;
    Policy policy = FIFO;
    std::vector<SegConfig> segs;
};

std::string validateConfig(const Config& c) {
    if (c.frames < 1 || c.frames > 4096) return "Number of frames must be 1 to 4096";
    if (c.pageSize < 16 || c.pageSize > 65536) return "Page size must be 16 to 65536";
    if (c.dirSize < 1 || c.dirSize > 64) return "Pages per directory table must be 1 to 64";
    if (c.segs.empty() || c.segs.size() > 32) return "Need 1 to 32 segments";
    long long region = (long long)c.frames * c.pageSize;
    long long totalPages = 0;
    int preloadCount = 0;
    for (size_t i = 0; i < c.segs.size(); ++i) {
        const SegConfig& s = c.segs[i];
        std::string name = "Segment " + std::to_string(s.id);
        if (s.id != (int)i) return "Segment IDs must go 0, 1, 2... in order (found " + std::to_string(s.id) +
                                   " at position " + std::to_string(i) + ")";
        if (s.limit < 1 || s.limit > 256) return name + " limit must be 1 to 256 pages";
        if (s.base < 0) return name + " has a negative base address";
        if (i > 0 && s.base < c.segs[i - 1].base + region) {
            return name + " base " + std::to_string(s.base) + " overlaps segment " + std::to_string(i - 1) +
                   " (needs to be at least " + std::to_string(c.segs[i - 1].base + region) + ")";
        }
        for (const auto& p : s.preload) {
            if (p.first < 0 || p.first >= s.limit) {
                return name + " preload page " + std::to_string(p.first) + " is outside its limit of " +
                       std::to_string(s.limit);
            }
        }
        totalPages += s.limit;
        preloadCount += (int)s.preload.size();
    }
    if (preloadCount > c.frames) {
        return "Preloaded pages (" + std::to_string(preloadCount) + ") do not fit in " +
               std::to_string(c.frames) + " frames";
    }
    if (totalPages > (long long)c.frames * MAX_OVERCOMMIT) {
        return "Total pages (" + std::to_string(totalPages) + ") is more than " + std::to_string(MAX_OVERCOMMIT) +
               "x the number of frames (" + std::to_string(c.frames) + "). Add frames or use fewer pages.";
    }
    if (c.segs.back().base + region > std::numeric_limits<int>::max()) {
        return "Address space is too big for this simulator";
    }
    return "";
}

bool loadConfigFile(const std::string& fileName, Config& cfg, std::string& err) {
    std::ifstream in(fileName);
    if (!in) {
        err = "Could not open " + fileName;
        return false;
    }
    std::string line;
    int lineNum = 0;
    while (std::getline(in, line)) {
        lineNum++;
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream ss(line);
        std::string word;
        if (!(ss >> word)) continue;
        std::string where = "Line " + std::to_string(lineNum) + ": ";
        if (word == "frames") {
            if (!(ss >> cfg.frames)) { err = where + "bad frames value"; return false; }
        } else if (word == "pagesize") {
            if (!(ss >> cfg.pageSize)) { err = where + "bad pagesize value"; return false; }
        } else if (word == "dirsize") {
            if (!(ss >> cfg.dirSize)) { err = where + "bad dirsize value"; return false; }
        } else if (word == "policy") {
            std::string p;
            ss >> p;
            if (p == "FIFO" || p == "fifo") cfg.policy = FIFO;
            else if (p == "LRU" || p == "lru") cfg.policy = LRU;
            else { err = where + "policy must be FIFO or LRU"; return false; }
        } else {
            std::istringstream segLine(line);
            if (word == "segment") segLine >> word;
            SegConfig s;
            std::string protText;
            if (!(segLine >> s.id >> s.base >> s.limit >> protText) || !parseProt(protText, s.prot)) {
                err = where + "segment line should be: segID base limit RO|RW [page:RO|RW ...]";
                return false;
            }
            s.randomPages = false;
            std::string tok;
            while (segLine >> tok) {
                size_t colon = tok.find(':');
                Protection pp = READ_WRITE;
                std::string num = tok.substr(0, colon);
                if (colon != std::string::npos && !parseProt(tok.substr(colon + 1), pp)) {
                    err = where + "bad page protection in " + tok;
                    return false;
                }
                try {
                    s.preload.push_back({std::stoi(num), pp});
                } catch (...) {
                    err = where + "bad page number in " + tok;
                    return false;
                }
            }
            cfg.segs.push_back(s);
        }
    }
    err = validateConfig(cfg);
    return err.empty();
}

void clearLine() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int readInt(const std::string& prompt, int lo, int hi) {
    int v;
    while (true) {
        std::cout << prompt;
        if (std::cin >> v && v >= lo && v <= hi) return v;
        if (std::cin.eof()) exit(0);
        clearLine();
        std::cout << "Please enter a number from " << lo << " to " << hi << ".\n";
    }
}

double readDouble(const std::string& prompt, double lo, double hi) {
    double v;
    while (true) {
        std::cout << prompt;
        if (std::cin >> v && v >= lo && v <= hi) return v;
        if (std::cin.eof()) exit(0);
        clearLine();
        std::cout << "Please enter a number from " << lo << " to " << hi << ".\n";
    }
}

char readYesNo(const std::string& prompt) {
    std::string s;
    while (true) {
        std::cout << prompt;
        if (!(std::cin >> s)) exit(0);
        if (s == "y" || s == "Y") return 'y';
        if (s == "n" || s == "N") return 'n';
        std::cout << "Please type y or n.\n";
    }
}

Config askForConfig() {
    while (true) {
        Config cfg;
        if (readYesNo("Load configuration from a file? (y/n): ") == 'y') {
            std::string fileName;
            std::cout << "Config file name: ";
            std::cin >> fileName;
            std::string err;
            if (loadConfigFile(fileName, cfg, err)) {
                std::cout << "Loaded " << cfg.segs.size() << " segments from " << fileName << "\n";
                return cfg;
            }
            std::cout << "Config error: " << err << "\n";
            continue;
        }

        cfg.frames = readInt("Enter number of physical frames: ", 1, 4096);
        cfg.pageSize = readInt("Enter page size: ", 16, 65536);
        cfg.dirSize = readInt("Enter pages per directory table: ", 1, 64);
        int numSegments = readInt("Enter number of segments: ", 1, 32);
        int pol = readInt("Replacement policy (1=FIFO, 2=LRU): ", 1, 2);
        cfg.policy = (pol == 1) ? FIFO : LRU;
        bool manualPages = readYesNo("Enter pages for each segment by hand? (y/n): ") == 'y';

        long long region = (long long)cfg.frames * cfg.pageSize;
        for (int i = 0; i < numSegments; ++i) {
            SegConfig s;
            s.id = i;
            s.base = i * region;
            s.randomPages = true;
            if (manualPages) {
                s.limit = readInt("  Segment " + std::to_string(i) + " number of pages: ", 1, 256);
                int p = readInt("  Segment " + std::to_string(i) + " protection (0=RO, 1=RW): ", 0, 1);
                s.prot = p ? READ_WRITE : READ_ONLY;
            } else {
                s.limit = 3 + rand() % 5;
                s.prot = (rand() % 2) ? READ_ONLY : READ_WRITE;
            }
            cfg.segs.push_back(s);
        }

        std::string err = validateConfig(cfg);
        if (err.empty()) return cfg;
        std::cout << "Config error: " << err << "\nLet's try again.\n";
    }
}

void saveLatencyHistory(const Metrics& m) {
    if (m.translations == 0) return;
    double avg = (double)m.totalLatency / m.translations;
    std::ofstream out("latency_history.txt", std::ios::app);
    out << avg << " " << m.translations << "\n";
    out.close();

    std::ifstream in("latency_history.txt");
    double a;
    long long n;
    double weighted = 0;
    long long count = 0;
    int runs = 0;
    while (in >> a >> n) {
        weighted += a * n;
        count += n;
        runs++;
    }
    std::cout << "Average latency this run: " << avg << " units\n";
    if (count > 0) {
        std::cout << "Average latency across " << runs << " runs: " << weighted / count << " units\n";
    }
}

void manualTranslations(SegmentTable& segmentTable) {
    std::cout << "Enter logical address (seg, dir, page, offset, access[0=read,1=write]) or -1 to stop: ";
    int segNum, pageDir, pageNum, offset, access;
    while (std::cin >> segNum) {
        if (segNum == -1) break;
        if (!(std::cin >> pageDir >> pageNum >> offset >> access)) {
            clearLine();
            std::cout << "Bad input. Enter 5 numbers or -1: ";
            continue;
        }
        int latency;
        int physicalAddress = segmentTable.translateAddress(segNum, pageDir, pageNum, offset,
                                                            access ? READ_WRITE : READ_ONLY, latency);
        if (physicalAddress != -1) {
            std::cout << "Physical Address: " << physicalAddress << ", Latency: " << latency;
            if (segmentTable.lastFault) {
                std::cout << (segmentTable.lastReplaced ? " (page fault, replaced a page)" : " (page fault, used a free frame)");
            }
            std::cout << "\n";
        } else {
            std::cout << segmentTable.lastError << "\n";
        }
        std::cout << "Physical Memory Utilization: " << segmentTable.physMem.utilization() << "%\n";
        std::cout << "Enter next address or -1: ";
    }
    segmentTable.printMemoryMap();
}

int main() {
    srand(time(0));
    std::cout << "=== Segmented, Paged Memory Simulator ===\n";
    Config cfg = askForConfig();

    SegmentTable segmentTable(cfg.frames, cfg.pageSize, cfg.dirSize, cfg.policy);
    for (const auto& s : cfg.segs) {
        segmentTable.addSegment(s.id, (int)s.base, s.limit, s.prot, s.randomPages);
        for (const auto& p : s.preload) segmentTable.preloadPage(s.id, p.first, p.second);
    }

    long long totalPages = 0;
    for (const auto& s : cfg.segs) totalPages += s.limit;
    if (totalPages > cfg.frames) {
        std::cout << "Note: " << totalPages << " pages but only " << cfg.frames
                  << " frames, so " << policyName(cfg.policy) << " replacement will be used.\n";
    }

    segmentTable.printMemoryMap();

    while (true) {
        std::cout << "\nMenu:\n"
                  << "  1. Translate addresses by hand\n"
                  << "  2. Process a batch file\n"
                  << "  3. Run stress test\n"
                  << "  4. Show memory map\n"
                  << "  5. Show timeline\n"
                  << "  6. Show metrics\n"
                  << "  0. Quit\n";
        int choice = readInt("Choice: ", 0, 6);
        if (choice == 0) break;
        if (choice == 1) {
            manualTranslations(segmentTable);
        } else if (choice == 2) {
            std::string fileName;
            std::cout << "Batch file name: ";
            std::cin >> fileName;
            processBatch(segmentTable, fileName);
        } else if (choice == 3) {
            int count = readInt("How many addresses? (100 is normal): ", 1, 100000);
            double ratio = readDouble("Valid ratio (0.0 to 1.0): ", 0.0, 1.0);
            generateRandomAddresses(segmentTable, count, ratio, "results.txt");
            std::cout << "Results logged to results.txt\n";
        } else if (choice == 4) {
            segmentTable.printMemoryMap();
        } else if (choice == 5) {
            segmentTable.printTimeline();
        } else if (choice == 6) {
            segmentTable.printMetrics();
        }
    }

    segmentTable.printMetrics();
    saveLatencyHistory(segmentTable.metrics);
    std::cout << "Errors were logged to errors.txt\n";
    return 0;
}
