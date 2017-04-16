#include "UnityPrefix.h"

#if ENABLE_UNIT_TESTS

#include "Configuration/UnityConfigure.h"
#include "Runtime/Testing/Testing.h"
#include "Runtime/Testing/TestFixtures.h"
#include "Runtime/Jobs/Jobs.h"
#include "Runtime/Jobs/JobBatchDispatcher.h"
#include "Runtime/Math/Random/Random.h"
#include "Runtime/Utilities/Hash128.h"
#include "Runtime/Utilities/SpookyV2.h"

#define RUN_OVER_NIGHT 0
#define DEBUG_JOB_QUEUE_STRESS_TESTS 0

#if DEBUG_JOB_QUEUE_STRESS_TESTS
static void JobQueueStressTestsDebug(const char* msg,...)
{
	va_list vl;
	va_start(vl, msg);
	char buffer[1024 * 4] = { 0 };
	vsnprintf (buffer, sizeof(buffer), msg, vl);
	va_end(vl);
	
#if UNITY_WIN
	OutputDebugString(buffer);
#else 
	print_realstdout(buffer, strlen(buffer));
#endif 
}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

SUITE (JobQueueStressTests)
{
	enum { kMaxForEach = 100, kMaxDependencyCount = 100 };
	enum Mode { kNotSet, kSyncFenceAndCheck, kDirectCall, kJob, kJobForEach, kJobForEachWithCombine };
	enum SyncFenceMode { kBackToFront, kFrontToBack };

	template<class T>
	inline void HashValue (const T& value, Hash128& hashInOut)
	{
		SpookyHash::Hash128 (&value, sizeof (T), &hashInOut.hashData.u64[0], &hashInOut.hashData.u64[1]);
	}

	struct ScheduleInstruction
	{
		ScheduleInstruction()
		{
			Init();
		}
		void Init()
		{
			mode = kNotSet;
			dependsOn = NULL;
			forEachCount = -1;
			useDispatcher = false;

			syncFenceDependenciesCount = 0;
			for (int i = 0; i < kMaxForEach; ++i)
			{
				expectedValues[i] = -1;
				jobSetValues[i] = -2;
			}
			expectedCount = 0;

			for (int i = 0; i < kMaxDependencyCount; ++i)
			{
				syncFenceDependencies[i] = NULL;
				expectedCompletedJobs[i] = NULL;
			}
			expectedCompletedJobsCount = 0;

			expectsCombineJob = false;
			didRunCombineJob = false;
		}

		// Only used when scheduling the instruction, not used when the instruction is executing
		Mode		mode;
		JobFence*	dependsOn;
		int			forEachCount;
		bool		useDispatcher;

		// Used when the instruction is executing
		JobFence    thisFence;
		JobFence* 	syncFenceDependencies[kMaxDependencyCount];
		int         syncFenceDependenciesCount;

		// Expected value
		int 		expectedValues[kMaxForEach];
		int 		jobSetValues[kMaxForEach];
		int 		expectedCount;

		ScheduleInstruction* 	expectedCompletedJobs[kMaxDependencyCount];
		int 					expectedCompletedJobsCount;

		bool 					expectsCombineJob;
		bool 					didRunCombineJob;

	};

	static void EnsureInstructionHasSuccessfullyCompleted (const ScheduleInstruction& data)
	{
		for (int i = 0; i < data.expectedCount; i++)
		{
			CHECK_EQUAL(data.expectedValues[i], data.jobSetValues[i]);
		}

		CHECK_EQUAL(data.expectsCombineJob, data.didRunCombineJob);
	}

	static void SetJobValuesAndExpectDependenciesInternal (ScheduleInstruction* data)
	{
		CHECK(data->syncFenceDependenciesCount <= data->expectedCompletedJobsCount);
		for (int i = 0; i < data->syncFenceDependenciesCount; i++)
			SyncFence(*data->syncFenceDependencies[i]);

		for(int i = 0; i < data->expectedCompletedJobsCount; i++)
			EnsureInstructionHasSuccessfullyCompleted(*data->expectedCompletedJobs[i]);
		
		Assert(data->expectedCount == 1 || data->syncFenceDependenciesCount == 0);
		data->jobSetValues[0] = data->expectedValues[0];
	}

	static void SetJobValuesAndExpectDependencies (ScheduleInstruction* data)
	{
		SetJobValuesAndExpectDependenciesInternal(data);
	}

	static void SetJobValuesAndExpectDependenciesForEach (ScheduleInstruction* data, unsigned index)
	{
		Assert(data->syncFenceDependenciesCount == 0);

		for(int i = 0; i < data->expectedCompletedJobsCount; i++)
			EnsureInstructionHasSuccessfullyCompleted(*data->expectedCompletedJobs[i]);
		
		data->jobSetValues[index] = data->expectedValues[index];
	}

	static void SetJobValuesAndExpectDependenciesForEachCombine (ScheduleInstruction* data)
	{
		for(int i = 0; i < data->expectedCompletedJobsCount; i++)
			EnsureInstructionHasSuccessfullyCompleted(*data->expectedCompletedJobs[i]);
		
		CHECK_EQUAL(true, data->expectsCombineJob);
		CHECK_EQUAL(false, data->didRunCombineJob);
		data->didRunCombineJob = true;

		EnsureInstructionHasSuccessfullyCompleted(*data);
	}

	class SharedData
	{
		dynamic_array<ScheduleInstruction> 	instructions;
#if DEBUG_JOB_QUEUE_STRESS_TESTS
		typedef vector_map<const ScheduleInstruction*, int> InstructionMap;
		typedef InstructionMap::iterator InstructionMapIterator;
		typedef vector_map<const JobFence*, int> JobFenceMap;
		typedef JobFenceMap::iterator JobFenceMapIterator;

		InstructionMap instructionMap;
		JobFenceMap jobFenceMap;
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS
		
		public:
		SharedData() : instructions(kMemTempJobAlloc) {  }

		int GetScheduledInstructionCount() { return instructions.size(); }
		
#if DEBUG_JOB_QUEUE_STRESS_TESTS
		int FindInstructionIndexFromJobFence(const JobFence* const pFence)
		{
			if (pFence == NULL)
			{
				return -1;
			}
			JobFenceMapIterator found = jobFenceMap.find(pFence);
			CHECK(found != jobFenceMap.end());
			const int index = found->second;
			return index;
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		int FindInstructionIndexFromInstruction(const ScheduleInstruction* const instruction)
		{
			InstructionMapIterator found = instructionMap.find(instruction);
			CHECK(found != instructionMap.end());
			const int index = found->second;
			return index;
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

		void Reserve(int count)
		{
			instructions.reserve(count);
#if DEBUG_JOB_QUEUE_STRESS_TESTS
			instructionMap.reserve(count);
			jobFenceMap.reserve(count);
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS
		}
		
		void SyncFenceAndCheck(int instructionIndex)
		{
			SyncFence(instructions[instructionIndex].thisFence);
			EnsureInstructionHasSuccessfullyCompleted(instructions[instructionIndex]);
		}

		void SyncFencesAndCheck(SyncFenceMode mode)
		{
			if (mode == kFrontToBack)
			{
				for (int i = 0;i < instructions.size();i++)
					SyncFenceAndCheck(i);
			}
			else if (mode == kBackToFront)
			{
				for (int i = instructions.size()-1;i >= 0;i--)
					SyncFenceAndCheck(i);
			}
			else
			{
				AssertMsg(false, "invalid mode SyncFencesAndCheck");
			}
		}

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		void ValidateInstructionPreExecute(const int instructionIndex, const ScheduleInstruction& instruction)
		{
			const int numSyncFences = instruction.syncFenceDependenciesCount;
			for (int s = 0; s < numSyncFences; ++s)
			{
				CheckValidJobFence(instruction.syncFenceDependencies[s], true);
			}
			for (int s = numSyncFences; s < kMaxDependencyCount; ++s)
			{
				CHECK(!instruction.syncFenceDependencies[s]);
			}

			const int numCompleted = instruction.expectedCompletedJobsCount;
			for (int c = 0; c < numCompleted; ++c)
			{
				const ScheduleInstruction* const expectedComplete = instruction.expectedCompletedJobs[c];
				CheckValidJobFence(&expectedComplete->thisFence, false);
			}
			for (int c = numCompleted; c < kMaxDependencyCount; ++c)
			{
				CHECK(!instruction.expectedCompletedJobs[c]);
			}

			if (instruction.dependsOn)
				CheckValidJobFence(instruction.dependsOn, false);

			// jobSetValues must be at scheduled value
			const int expectedCount = instruction.expectedCount;
			CHECK(expectedCount >= 0);
			const int forEachCount = instruction.forEachCount;
			CHECK(forEachCount >= 0);
			for (int i = 0; i < forEachCount; ++i)
			{
				CHECK_EQUAL(instructionIndex, instruction.expectedValues[i]);
				CHECK_EQUAL(-3, instruction.jobSetValues[i]);
			}
			for (int i = forEachCount; i < kMaxForEach; ++i)
			{
				CHECK_EQUAL(-1, instruction.expectedValues[i]);
				CHECK_EQUAL(-2, instruction.jobSetValues[i]);
			}

		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

		bool IsValidInstructionToSyncOn(const ScheduleInstruction* const instruction)
		{
			// Invalid to sync on an instruction which calls SyncFence (directly)
			if (0 != instruction->syncFenceDependenciesCount)
				return false;

			// The dependent instruction is always the last entry in instruction.expectedCompletedJobs
			int expectedCompletedJobsCount = instruction->expectedCompletedJobsCount;
			if (expectedCompletedJobsCount == 0)
				return true;

			//Invalid to depend on an instruction which calls SyncFence (directly or indirectly)
			const ScheduleInstruction* const dependency = instruction->expectedCompletedJobs[expectedCompletedJobsCount-1];
			return IsValidInstructionToSyncOn(dependency);
		}

		void CreateInstruction(const Mode mode, const int chainDependency, const int* const fenceDependencies, const int maxFenceDependenciesCount, const int forEachCount, const bool useDispatcher)
		{
			Assert(instructions.size() < instructions.capacity());
			
			ScheduleInstruction& instruction = instructions.push_back_construct();

			if ((mode == kDirectCall) || (mode == kJobForEach) || (mode == kJobForEachWithCombine))
			{
				Assert(maxFenceDependenciesCount == 0);
			}

			// Setup fences and expected dependency info
			int fenceDependenciesCount = 0;
			for (int i = 0; i != maxFenceDependenciesCount;i++)
			{
				Assert(fenceDependencies[i] < instructions.size() - 1);
				ScheduleInstruction* syncInstruction = &instructions[fenceDependencies[i]];
				if (IsValidInstructionToSyncOn(syncInstruction))
				{
					instruction.expectedCompletedJobs[fenceDependenciesCount] = syncInstruction;
					instruction.syncFenceDependencies[fenceDependenciesCount] = &syncInstruction->thisFence;
					++fenceDependenciesCount;
				}
			}

			instruction.syncFenceDependenciesCount = fenceDependenciesCount;
			instruction.expectedCompletedJobsCount = fenceDependenciesCount;
			const int thisInstructionIndex = instructions.size()-1;
			JobFence* dependsOn = NULL;
			if (chainDependency >= 0)
			{
				Assert(chainDependency <= thisInstructionIndex);

				instruction.expectedCompletedJobs[instruction.expectedCompletedJobsCount] = &instructions[chainDependency];
				instruction.expectedCompletedJobsCount += 1;
				dependsOn = &instructions[chainDependency].thisFence;
			}

			for (int i = 0; i != forEachCount;i++)
			{
				instruction.expectedValues[i] = thisInstructionIndex;
				instruction.jobSetValues[i] = -3;
			}

			instruction.expectedCount = forEachCount;
			instruction.didRunCombineJob = false;
			instruction.expectsCombineJob = (mode == kJobForEachWithCombine);

			// Only used to execute/schedule the instruction, not used when the instruction is run
			instruction.mode = mode;
			instruction.dependsOn = dependsOn;
			instruction.forEachCount = forEachCount;
			instruction.useDispatcher = useDispatcher;
		}

		void ExecuteInstruction(const int instructionIndex, ScheduleInstruction& instruction, int& numInDispatcher, JobBatchDispatcher* pDispatcher)
		{
			const Mode mode = instruction.mode;
			const bool useDispatcher = instruction.useDispatcher;
			const bool useDependsOn = (instruction.dependsOn != NULL);
			const int forEachCount = instruction.forEachCount;
			JobFence* dependsOn = instruction.dependsOn;

			if (useDispatcher)
				++numInDispatcher;
			else if (numInDispatcher > 0)
			{
				pDispatcher->KickJobs();
				numInDispatcher = 0;
			}

#if DEBUG_JOB_QUEUE_STRESS_TESTS
			ValidateInstructionPreExecute(instructionIndex, instruction);
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

			if (mode == kSyncFenceAndCheck)
			{
				if (useDependsOn)
					SyncFence(*dependsOn);
				SetJobValuesAndExpectDependenciesInternal (&instruction);
			}
			else if (mode == kDirectCall)
			{
				if (useDependsOn)
					SyncFence(*dependsOn);
				SetJobValuesAndExpectDependenciesInternal (&instruction);
			}
			else if (mode == kJob)
			{
				if (useDispatcher)
				{
					if (useDependsOn)
						pDispatcher->ScheduleJobDepends(instruction.thisFence, SetJobValuesAndExpectDependencies, &instruction, *dependsOn);
					else
						pDispatcher->ScheduleJob(instruction.thisFence, SetJobValuesAndExpectDependencies, &instruction);
				}
				else
				{
					if (useDependsOn)
						ScheduleJobDepends(instruction.thisFence, SetJobValuesAndExpectDependencies, &instruction, *dependsOn);
					else
						ScheduleJob(instruction.thisFence, SetJobValuesAndExpectDependencies, &instruction);
				}
			}
			else if (mode == kJobForEach)
			{
				if (useDispatcher)
				{
					if (useDependsOn)
						pDispatcher->ScheduleJobForEachDepends(instruction.thisFence, SetJobValuesAndExpectDependenciesForEach, &instruction, forEachCount, *dependsOn);
					else
						pDispatcher->ScheduleJobForEach(instruction.thisFence, SetJobValuesAndExpectDependenciesForEach, &instruction, forEachCount);
				}
				else
				{
					if (useDependsOn)
						ScheduleJobForEachDepends(instruction.thisFence, SetJobValuesAndExpectDependenciesForEach, &instruction, forEachCount, *dependsOn);
					else
						ScheduleJobForEach(instruction.thisFence, SetJobValuesAndExpectDependenciesForEach, &instruction, forEachCount);
				}
			}
			else if (mode == kJobForEachWithCombine)
			{
				if (useDispatcher)
				{
					if (useDependsOn)
						pDispatcher->ScheduleJobForEachDepends(instruction.thisFence, SetJobValuesAndExpectDependenciesForEach, &instruction, forEachCount, *dependsOn, SetJobValuesAndExpectDependenciesForEachCombine);	
					else
						pDispatcher->ScheduleJobForEach(instruction.thisFence, SetJobValuesAndExpectDependenciesForEach, &instruction, forEachCount, SetJobValuesAndExpectDependenciesForEachCombine);	
				}
				else
				{
					if (useDependsOn)
						ScheduleJobForEachDepends(instruction.thisFence, SetJobValuesAndExpectDependenciesForEach, &instruction, forEachCount, *dependsOn, SetJobValuesAndExpectDependenciesForEachCombine);	
					else
						ScheduleJobForEach(instruction.thisFence, SetJobValuesAndExpectDependenciesForEach, &instruction, forEachCount, SetJobValuesAndExpectDependenciesForEachCombine);	
				}
			}
			else
			{
				AssertMsg(false, "invalid mode in Execute");
			}
		}

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		void LogInstruction(const ScheduleInstruction& instruction, const int index, FILE* f)
		{
			const int mode = instruction.mode;
			const int foreachCount = instruction.forEachCount;
			fprintf(f, "ID%d Mode:%d ForEach:%d DependsOn ID%d", index, mode, foreachCount, FindInstructionIndexFromJobFence(instruction.dependsOn));

			const int numSyncFences = instruction.syncFenceDependenciesCount;
			fprintf(f, " NumSync:%d", numSyncFences);
			for (int s = 0; s < numSyncFences; ++s)
			{
				const int syncID = FindInstructionIndexFromJobFence(instruction.syncFenceDependencies[s]);
				fprintf(f, " ID%d", syncID);
			}

			const int numCompleted = instruction.expectedCompletedJobsCount;
			fprintf(f, " NumCompleted:%d", numCompleted);
			for (int c = 0; c < numCompleted; ++c)
			{
				const int syncID = FindInstructionIndexFromInstruction(instruction.expectedCompletedJobs[c]);
				fprintf(f, " ID%d", syncID);
			}
			fprintf(f, "\n");
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		void CheckValidJobFence(const JobFence* const jobFence, const bool checkSyncFenceChain)
		{
			const int syncID = FindInstructionIndexFromJobFence(jobFence);
			if ((instructions[syncID].mode == kDirectCall) || (instructions[syncID].mode == kSyncFenceAndCheck))
				return;
			// Job might already be completed so can't test the group.info field
			//CHECK(NULL != jobFence->group.info);
			CHECK(0u != jobFence->group.version);
			if (checkSyncFenceChain)
			{
				CHECK_EQUAL(0, instructions[syncID].syncFenceDependenciesCount);
			}
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		bool FindSyncFenceInCompletedJobs(const ScheduleInstruction& instruction, JobFence* syncFence)
		{
			const int numCompleted = instruction.expectedCompletedJobsCount;
			for (int c = 0; c < numCompleted; ++c)
			{
				ScheduleInstruction* expectedComplete = instruction.expectedCompletedJobs[c];
				if (syncFence == &expectedComplete->thisFence)
				{
					return true;
				}
			}
			return false;
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		void ValidateInstructionPostExecute(const int instructionIndex)
		{
			const ScheduleInstruction& instruction = instructions[instructionIndex];
			const int mode = instruction.mode;
			CHECK((mode == kSyncFenceAndCheck) || (mode == kDirectCall) || (mode == kJob) || (mode == kJobForEach) || (mode == kJobForEachWithCombine));
			const int numSyncFences = instruction.syncFenceDependenciesCount;
			CHECK(numSyncFences >= 0);
			for (int s = 0; s < numSyncFences; ++s)
			{
				const int syncID = FindInstructionIndexFromJobFence(instruction.syncFenceDependencies[s]);
				CHECK(syncID >= 0);
				CHECK(syncID < instructionIndex);
			}
			const int numCompleted = instruction.expectedCompletedJobsCount;
			CHECK(numCompleted >= numSyncFences);
			for (int c = 0; c < numCompleted; ++c)
			{
				ScheduleInstruction* expectedComplete = instruction.expectedCompletedJobs[c];
				const int syncID = FindInstructionIndexFromJobFence(&expectedComplete->thisFence);
				CHECK(syncID >= 0);
				CHECK(syncID < instructionIndex);
				const int expectedInstructionID = FindInstructionIndexFromInstruction(expectedComplete);
				CHECK(expectedInstructionID >= 0);
				CHECK_EQUAL(expectedInstructionID, syncID);
			}

			// if dependsOn isn't NULL then its fence must exist earlier in the instruction stream 
			if (instruction.dependsOn == NULL)
				CHECK_EQUAL(numCompleted, numSyncFences);
			else
			{
				const int syncID = FindInstructionIndexFromJobFence(instruction.dependsOn);
				CHECK(syncID >= 0);
				CHECK(syncID < instructionIndex);
			}

			// Expected value must be the instruction index
			const int expectedCount = instruction.expectedCount;
			for (int i = 0; i < expectedCount; ++i)
			{
				CHECK_EQUAL(instructionIndex, instruction.expectedValues[i]);
			}

			// All of the syncFenceDependencies must be in the thisFence of the completed jobs 
			int numSyncFencesFound = 0;
			for (int s = 0; s < numSyncFences; ++s)
			{
				if (FindSyncFenceInCompletedJobs(instruction, instruction.syncFenceDependencies[s]))
					++numSyncFencesFound;
			}
			CHECK_EQUAL(numSyncFences, numSyncFencesFound);

			// The dependsOn job must be in the completed jobs
			if (instruction.dependsOn)
			{
				CHECK_EQUAL(true, FindSyncFenceInCompletedJobs(instruction, instruction.dependsOn));
			}

			// All of the completed jobs must be in syncFenceDependencies or the dependsOn
			for (int c = 0; c < numCompleted; ++c)
			{
				bool found = false;
				JobFence* completedSyncFence = &instruction.expectedCompletedJobs[c]->thisFence;
				for (int s = 0; s < numSyncFences; ++s)
				{
					if (completedSyncFence == instruction.syncFenceDependencies[s])
					{
						found = true;
						break;
					}
				}
				if (found == false)
				{
					CHECK_EQUAL(instruction.dependsOn, completedSyncFence);
				}
			}
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		void ValidateProgram()
		{
			const int numInstructions = GetScheduledInstructionCount();
			for (int i = 0; i < numInstructions; ++i)
			{
				CHECK_EQUAL(i, FindInstructionIndexFromInstruction(&instructions[i]));
				ValidateInstructionPostExecute(i);
			}
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		void LogProgram(std::string& hashString)
		{
			static int fileID = 0;
			std::string fileName(Format("instructions%03d.txt", fileID));
			JobQueueStressTestsDebug("%s %s\n", fileName.c_str(), hashString.c_str());
			FILE* f = fopen(fileName.c_str(), "w");
			++fileID;
			const int numInstructions = GetScheduledInstructionCount();
			fprintf(f, "NumInstructions:%d Hash:%s\n", numInstructions, hashString.c_str());
			for (int i = 0; i < numInstructions; ++i)
			{
				const ScheduleInstruction& instruction = instructions[i];
				LogInstruction(instruction, i, f);
			}
			fclose(f);
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		Hash128 HashProgram ()
		{
			Hash128 hash;
			const int numInstructions = GetScheduledInstructionCount();
			for (int i = 0; i < numInstructions; ++i)
			{
				const ScheduleInstruction& instruction = instructions[i];
				const int mode = instruction.mode;
				const int numSyncFences = instruction.syncFenceDependenciesCount;
				const int foreachCount = instruction.forEachCount;
				HashValue(mode, hash);
				HashValue(numSyncFences, hash);
				HashValue(foreachCount, hash);
			}
			return hash;
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		void PrepareValidationData()
		{
			const int numInstructions = GetScheduledInstructionCount();
			for (int i = 0; i < numInstructions; ++i)
			{
				CHECK_EQUAL(true, instructionMap.insert(std::make_pair(&instructions[i], i)).second);
				// thisFence : must be unique amongst all other instructions except NULL case
				CHECK_EQUAL(true, jobFenceMap.insert(std::make_pair(&instructions[i].thisFence, i)).second);
			}
		}
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS

		void ExecuteProgram()
		{
			JobBatchDispatcher dispatcher;
			const int numInstructions = GetScheduledInstructionCount();
			int numInDispatcher = 0; 
			for (int i = 0; i < numInstructions; ++i)
			{
				ScheduleInstruction& instruction = instructions[i];
				ExecuteInstruction(i, instruction, numInDispatcher, &dispatcher);
			}
		}
	};

	struct Config
	{
		int syncFenceChance;
		int jobChance;
		int jobForEachChance;
		int jobForEachWithCombineChance;
		int directCallChance;

		// usePreviousJobAsDependencyChanceo = 100 means when picking a dependency always pick the previous job;
		// usePreviousJobAsDependencyChanceo = 0 means when picking a dependency it is pure random
		int usePreviousJobAsDependencyChance;

		// hasDependencyChance = 100 means always
		// hasDependencyChance = 0 means never
		int hasDependencyChance;

		// jobBatchDispatcherChance = 100 means always
		// jobBatchDispatcherChance = 0 means never
		int jobBatchDispatcherChance;

		Config ()
		{
			syncFenceChance = 2;
			jobChance = 50;
			jobForEachChance = 0; // 20;
			jobForEachWithCombineChance = 0; // 10;
			directCallChance = 2;

			usePreviousJobAsDependencyChance = 50;
			hasDependencyChance = 50;
			jobBatchDispatcherChance = 50;
		}
		void Zero()
		{
			syncFenceChance = 0;
			jobChance = 0;
			jobForEachChance = 0;
			jobForEachWithCombineChance = 0;
			directCallChance = 0;

			usePreviousJobAsDependencyChance = 0;
			hasDependencyChance = 0;
			jobBatchDispatcherChance = 0;
		}

		void ClearSyncFenceAndDirectCall()
		{
			directCallChance = 0;
			syncFenceChance = 0;
		}

		void SumChances()
		{
			jobChance += syncFenceChance;
			jobForEachChance += jobChance;
			jobForEachWithCombineChance += jobForEachChance;
			directCallChance += jobForEachWithCombineChance;
		}

	};

	void CreateRandomSet (Rand& rand, SharedData& data, Config config, int jobCount)
	{
		Assert(jobCount > 0);
		config.SumChances();

		enum { kMaxSyncFenceDependencies = 10 };
		
		data.Reserve(jobCount);

		// The first instruction is always job or directCall with forEachCount = 0 (no previous jobs to expect or wait for)
		const Mode mode = (config.jobForEachWithCombineChance > config.syncFenceChance) ? kJob : kDirectCall;
		int dependency = -1;
		int syncDependencies[kMaxSyncFenceDependencies];
		int syncDependenciesCount = 0;
		data.CreateInstruction(mode, dependency, syncDependencies, syncDependenciesCount, 1, false);

		for (int i = 1; i < jobCount; ++i)
		{
			const int numScheduledInstructions = data.GetScheduledInstructionCount();
			const int type = RangedRandom(rand, 0, config.directCallChance);
			const int foreachCount = RangedRandom(rand, 1, kMaxForEach+1);
			bool useDispatcher = RangedRandom (rand, 0, 100) < config.jobBatchDispatcherChance;
			const bool useDependency = RangedRandom(rand, 0, 100) < config.hasDependencyChance;
			const bool usePreviousJobAsDependency = RangedRandom(rand, 0, 100) < config.usePreviousJobAsDependencyChance;
			const int randomDependency = RangedRandom(rand, 0, numScheduledInstructions);
			const int randomSyncDependency = RangedRandom(rand, 0, numScheduledInstructions);

			if (config.jobBatchDispatcherChance == 100)
			{
				Assert(useDispatcher);
			}
			else if (config.jobBatchDispatcherChance == 0)
			{
				Assert(!useDispatcher);
			}

			// Build a random dependency
			const int jobDependency = usePreviousJobAsDependency ? (numScheduledInstructions-1) : randomDependency;
			dependency = useDependency ? jobDependency : -1;

			// Build random array of sync fence dependencies
			syncDependenciesCount = RangedRandom(rand, 0, kMaxSyncFenceDependencies+1);
			for (int d = 0; d < syncDependenciesCount; d++)
				syncDependencies[d] = RangedRandom(rand, 0, numScheduledInstructions);
		
			if (type < config.syncFenceChance)
			{
				if (syncDependenciesCount == 0)
				{
					syncDependenciesCount = 1;
					syncDependencies[0] = randomSyncDependency;
				}
				data.CreateInstruction(kSyncFenceAndCheck, dependency, syncDependencies, syncDependenciesCount, 1, false);
			}
			else if (type < config.jobChance)
			{
				data.CreateInstruction(kJob, dependency, syncDependencies, syncDependenciesCount, 1, useDispatcher);
			}
			else if (type < config.jobForEachChance)
			{
				data.CreateInstruction(kJobForEach, dependency, NULL, 0, foreachCount, useDispatcher);
			}
			else if (type < config.jobForEachWithCombineChance)
			{
				data.CreateInstruction(kJobForEachWithCombine, dependency, NULL, 0, foreachCount, useDispatcher);
			}
			else if (type < config.directCallChance)
			{
				data.CreateInstruction(kDirectCall, dependency, NULL, 0, 1, false);
			}
			else
			{
				AssertMsg(false, "invalid type value");
			}
		}
#if DEBUG_JOB_QUEUE_STRESS_TESTS
		JobQueueStressTestsDebug(".");
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS
	}

	void ExecuteProgram(SharedData& data)
	{
#if DEBUG_JOB_QUEUE_STRESS_TESTS
		data.PrepareValidationData();
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS
		data.ExecuteProgram();

#if DEBUG_JOB_QUEUE_STRESS_TESTS
		// Do this on the main thread to keep the main thread busy and get some overlapped work
		Hash128 programHash = data.HashProgram ();
		std::string programHashString = Hash128ToString(programHash);
		data.LogProgram(programHashString);
		data.ValidateProgram();
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS
	}

	void ScheduleRandomSet (Rand& rand, SharedData& data, Config config, int jobCount)
	{
		CreateRandomSet(rand, data, config, jobCount);
		ExecuteProgram(data);
	}

	TEST (ScheduleJob_NoDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (ScheduleJob_RandomDepends)
	{
		enum { kJobsPerSet = 15 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 0;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (ScheduleJob_PrevDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_ScheduleJob_NoDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobChance = 100;
		config.jobBatchDispatcherChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_ScheduleJob_RandomDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobChance = 100;
		config.jobBatchDispatcherChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 0;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_ScheduleJob_PrevDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobChance = 100;
		config.jobBatchDispatcherChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (JobForEach_NoDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (JobForEach_RandomDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 0;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (JobForEach_PrevDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_JobForEach_NoDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachChance = 100;
		config.jobBatchDispatcherChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_JobForEach_RandomDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachChance = 100;
		config.jobBatchDispatcherChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 0;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_JobForEach_PrevDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachChance = 100;
		config.jobBatchDispatcherChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (JobForEachWithCombine_NoDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachWithCombineChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (JobForEachWithCombine_RandomDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachWithCombineChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 0;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (JobForEachWithCombine_PrevDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachWithCombineChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_JobForEachWithCombine_NoDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachWithCombineChance = 100;
		config.jobBatchDispatcherChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_JobForEachWithCombine_RandomDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachWithCombineChance = 100;
		config.jobBatchDispatcherChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 0;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (BatchDispatcher_JobForEachWithCombine_PrevDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.jobForEachWithCombineChance = 100;
		config.jobBatchDispatcherChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (SyncFence_NoDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.syncFenceChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (SyncFence_RandomDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.syncFenceChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 0;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (SyncFence_PrevDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.syncFenceChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (DirectCall_NoDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.directCallChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (DirectCall_RandomDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.directCallChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 0;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	TEST (DirectCall_PrevDepends)
	{
		enum { kJobsPerSet = 40 };

		Rand rand;
		SharedData data;
		Config config;
		config.Zero();
		config.directCallChance = 100;
		config.hasDependencyChance = 100;
		config.usePreviousJobAsDependencyChance = 100;
		ScheduleRandomSet(rand, data, config, kJobsPerSet);
		data.SyncFencesAndCheck(kBackToFront);
	}

	void RunRandomStressTest(const int numJobsPerSet, const int numLoops)
	{
		Rand rand;

		for (int i = 0; i < numLoops; ++i)
		{
#if DEBUG_JOB_QUEUE_STRESS_TESTS
			JobQueueStressTestsDebug("\n%d", i);
#endif // #if DEBUG_JOB_QUEUE_STRESS_TESTS
			enum { kNumConfigs = 8 };
			SharedData data[kNumConfigs];
			Config config[kNumConfigs];
			int c = 0;

			config[c].jobBatchDispatcherChance = 0;
			CreateRandomSet(rand, data[c], config[c], numJobsPerSet);
			++c;

			config[c].ClearSyncFenceAndDirectCall();
			CreateRandomSet(rand, data[c], config[c], numJobsPerSet);
			++c;

			config[c].jobBatchDispatcherChance = 50;
			CreateRandomSet(rand, data[c], config[c], numJobsPerSet);
			++c;

			config[c].ClearSyncFenceAndDirectCall();
			config[c].jobBatchDispatcherChance = 50;
			CreateRandomSet(rand, data[c], config[c], numJobsPerSet);
			++c;

			config[c].jobBatchDispatcherChance = 100;
			CreateRandomSet(rand, data[c], config[c], numJobsPerSet);
			++c;

			config[c].ClearSyncFenceAndDirectCall();
			config[c].jobBatchDispatcherChance = 100;
			CreateRandomSet(rand, data[c], config[c], numJobsPerSet);
			++c;

			config[c].jobBatchDispatcherChance = 0;
			CreateRandomSet(rand, data[c], config[c], numJobsPerSet);
			++c;

			config[c].ClearSyncFenceAndDirectCall();
			config[c].jobBatchDispatcherChance = 0;
			CreateRandomSet(rand, data[c], config[c], numJobsPerSet);
			++c;

			Assert(c == kNumConfigs);

			for (c = 0; c < kNumConfigs; ++c)
			{
				ExecuteProgram(data[c]);
			}
			for (c = 0; c < kNumConfigs; ++c)
			{
				data[c].SyncFencesAndCheck((c & 0x1) ? kFrontToBack : kBackToFront);
			}
		}
	}

	void RunQuickRandomStressTest()
	{
		enum { kJobsPerSet = 20000 };
		enum { kLoopCount = 5 };
		RunRandomStressTest(kJobsPerSet, kLoopCount);
	}

	TEST(Random_Quick)
	{
		RunQuickRandomStressTest();
	}

#if RUN_OVER_NIGHT 
	TEST(Random_Long)
	{
		enum { kJobsPerSet = 20000 };
		// Takes ~15mins
		//enum { kLoopCount = 500 };
		// Takes ~60mins
		enum { kLoopCount = 2000 };
		RunRandomStressTest(kJobsPerSet, kLoopCount);
	}
#endif // #if RUN_OVER_NIGHT 
}

#endif
