//===-- VerboseTracer.h - Verbose Execution Tracing for GKLEE ---*- C++ -*-===//
//
// GKLEE Verbose Tracing
//
// This file provides verbose execution tracing for debugging and analysis.
// When enabled via --verbose <filename>, it traces:
// - LLVM instructions executed by each thread
// - Memory read/write operations with addresses
// - Race checking details at barriers
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_VERBOSETRACER_H
#define KLEE_VERBOSETRACER_H

#include <fstream>
#include <string>
#include <sstream>

namespace klee {

// Helper function to convert CTYPE (integer) to string
// CTYPE values: UNKNOWN=0, LOCAL=1, SHARED=2, DEVICE=3, HOST=4
inline std::string ctypeToString(int ctype) {
  switch (ctype) {
    case 0: return "UNKNOWN";
    case 1: return "LOCAL";
    case 2: return "SHARED";
    case 3: return "DEVICE";
    case 4: return "HOST";
    default: return "UNKNOWN";
  }
}

class VerboseTracer {
private:
  std::ofstream traceFile;
  bool enabled;
  unsigned currentTid;
  unsigned currentBarrierNum;

public:
  VerboseTracer() : enabled(false), currentTid(0), currentBarrierNum(0) {}

  ~VerboseTracer() {
    if (traceFile.is_open()) {
      traceFile.close();
    }
  }

  bool init(const std::string &filename) {
    if (filename.empty()) {
      enabled = false;
      return false;
    }
    traceFile.open(filename.c_str());
    if (traceFile.is_open()) {
      enabled = true;
      traceFile << "=== GKLEE Verbose Execution Trace ===\n";
      traceFile << "=====================================\n\n";
      return true;
    }
    enabled = false;
    return false;
  }

  bool isEnabled() const { return enabled; }

  void setCurrentThread(unsigned tid) {
    if (!enabled) return;
    if (tid != currentTid) {
      traceFile << "\n~~>\n";  // Thread separator
      traceFile << "\n=== Thread " << tid << " (Barrier Interval "
                << currentBarrierNum << ") ===\n";
      currentTid = tid;
    }
  }

  void startBarrierInterval(unsigned barrierNum) {
    if (!enabled) return;
    currentBarrierNum = barrierNum;
    traceFile << "\n########## BARRIER INTERVAL " << barrierNum
              << " ##########\n";
  }

  void traceInstruction(const std::string &instStr, unsigned tid) {
    if (!enabled) return;
    setCurrentThread(tid);
    traceFile << "  [INST] " << instStr << "\n";
  }

  void traceLoad(const std::string &instStr, unsigned tid,
                 const std::string &address, const std::string &memType) {
    if (!enabled) return;
    setCurrentThread(tid);
    traceFile << "  [READ]  T" << tid << " " << memType
              << " addr=" << address << "\n";
    traceFile << "          inst: " << instStr << "\n";
  }

  void traceStore(const std::string &instStr, unsigned tid,
                  const std::string &address, const std::string &value,
                  const std::string &memType) {
    if (!enabled) return;
    setCurrentThread(tid);
    traceFile << "  [WRITE] T" << tid << " " << memType
              << " addr=" << address
              << " val=" << value << "\n";
    traceFile << "          inst: " << instStr << "\n";
  }

  void traceBarrierReached(unsigned tid, unsigned barrierCount) {
    if (!enabled) return;
    traceFile << "\n  [BARRIER] Thread " << tid
              << " reached barrier #" << barrierCount << "\n";
  }

  void traceAllThreadsAtBarrier(unsigned numThreads, unsigned barrierNum) {
    if (!enabled) return;
    traceFile << "\n############################################\n";
    traceFile << "# ALL " << numThreads << " THREADS AT BARRIER "
              << barrierNum << " #\n";
    traceFile << "############################################\n";
  }

  void traceRaceCheckStart(const std::string &memoryType) {
    if (!enabled) return;
    traceFile << "\n--- RACE CHECKING: " << memoryType << " ---\n";
  }

  void traceRaceCheckAccess(unsigned tid1, unsigned tid2,
                            const std::string &accessType1,
                            const std::string &accessType2,
                            const std::string &addr1, const std::string &addr2) {
    if (!enabled) return;
    traceFile << "  Checking T" << tid1 << "(" << accessType1 << ") vs T"
              << tid2 << "(" << accessType2 << ")\n";
    traceFile << "    addr1=" << addr1 << "\n";
    traceFile << "    addr2=" << addr2 << "\n";
  }

  void traceRaceFound(unsigned tid1, unsigned tid2,
                      const std::string &details) {
    if (!enabled) return;
    traceFile << "  *** RACE DETECTED between T" << tid1
              << " and T" << tid2 << ": " << details << " ***\n";
  }

  void traceRaceCheckComplete(const std::string &memoryType,
                              bool raceFound, unsigned pairsChecked) {
    if (!enabled) return;
    traceFile << "--- Race check complete for " << memoryType
              << ": " << pairsChecked << " pairs checked, "
              << (raceFound ? "RACE FOUND" : "no races") << " ---\n";
  }

  void traceMemoryAccessCollected(unsigned tid, const std::string &accessType,
                                  const std::string &address,
                                  const std::string &memType,
                                  unsigned instSeqNum) {
    if (!enabled) return;
    traceFile << "  [COLLECT] T" << tid << " " << accessType << " "
              << memType << " addr=" << address
              << " seq=" << instSeqNum << "\n";
  }

  void flush() {
    if (enabled && traceFile.is_open()) {
      traceFile.flush();
    }
  }

  void traceMessage(const std::string &msg) {
    if (!enabled) return;
    traceFile << msg << "\n";
  }
};

// Global tracer instance
extern VerboseTracer verboseTracer;

} // namespace klee

#endif // KLEE_VERBOSETRACER_H
