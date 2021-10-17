//
// C++ Interface: alignment
//
// Description: 
//
//
// Author: BUI Quang Minh, Steffen Klaere, Arndt von Haeseler <minh.bui@univie.ac.at>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <map>
#include <vector>
#include <bitset>
#include "pattern.h"
#include "ncl/ncl.h"
#include <utils/gzstream.h>
#include <utils/nametoidmap.h>

const double MIN_FREQUENCY          = 0.0001;
const double MIN_FREQUENCY_DIFF     = 0.00001;

class PhyloTree;

/** class storing results of symmetry tests */
class SymTestResult {
public:
    SymTestResult() {
        significant_pairs = included_pairs = excluded_pairs = 0;
        pvalue_binom = -1.0;
        max_stat = pvalue_maxdiv = pvalue_perm = 0.0;
    }
    
    /** compute pvalue using bionomial test */
    void computePvalue();
    
    int significant_pairs; // number of significant sequence pairs
    int included_pairs; // total number of included sequence pairs
    int excluded_pairs; // number of excluded sequence pairs
    double max_stat; // maximum of the pair statistics
    double pvalue_binom; // pvalue of binomial test of symmetry
    double pvalue_maxdiv; // p-value of the sequence pair with maximum divergence
    double pvalue_perm; // p-value of permutation test of symmetry
};

/** class storing all pairwise statistics */
class SymTestStat {
public:
    SymTestStat() {
        part = 0;
        seq1 = seq2 = 0;
        chi2_sym = 0.0;
        chi2_marsym = std::numeric_limits<double>::quiet_NaN();
        chi2_intsym = std::numeric_limits<double>::quiet_NaN();
        pval_sym = std::numeric_limits<double>::quiet_NaN();
        pval_marsym = std::numeric_limits<double>::quiet_NaN();
        pval_intsym = std::numeric_limits<double>::quiet_NaN();
    }
    int part; // partition ID
    int seq1, seq2; // ID of sequence 1 and 2
    double chi2_sym; // chi2 statistic test of symmetry
    double chi2_marsym; // chi2 statistic test of marginal symmetry
    double chi2_intsym; // chi2 statistic test of internal symmetry
    double pval_sym; // chi2 p-value test of symmetry
    double pval_marsym; // chi2 p-value test of marginal symmetry
    double pval_intsym; // chi2 p-value test of internal symmetry
};

std::ostream& operator<< (std::ostream& stream, const SymTestResult& res);

#ifdef USE_HASH_MAP
struct hashPattern {
    size_t operator()(const vector<StateType> &sp) const {
        size_t sum = 0;
        for (StateType state : sp ) {
            sum = state + (sum << 6) + (sum << 16) - sum;
        }
        return sum;
    }
};
typedef unordered_map<vector<StateType>, int, hashPattern> PatternIntMap;
#else
typedef map<vector<StateType>, int> PatternIntMap;
#endif


constexpr int EXCLUDE_GAP   = 1; // exclude gaps
constexpr int EXCLUDE_INVAR = 2; // exclude invariant sites
constexpr int EXCLUDE_UNINF = 4; // exclude uninformative sites

class PatternInfoVector;

class AlignmentSummary;

struct SequenceInfo {
    double percent_gaps;
    bool   failed;
    double pvalue;
};

/**
Multiple Sequence Alignment. Stored by a vector of site-patterns

        @author BUI Quang Minh, Steffen Klaere, Arndt von Haeseler <minh.bui@univie.ac.at>
 */
class Alignment : public vector<Pattern>, public CharSet, public StateSpace {
    friend class SuperAlignment;
    friend class SuperAlignmentUnlinked;
    friend class PatternInfoVector;

public:

    /**
            constructor
     */
    Alignment();

    /**
            constructor
            @param filename file name
            @param sequence_type type of the sequence, either "BIN", "DNA", "AA", or NULL
            @param intype (OUT) input format of the file
     */
    Alignment(const char *filename, const char *sequence_type, 
              InputType& intype, const string& model);

    /**
     constructor
     @param data_block nexus DATA block
     @param sequence_type type of the sequence, either "BIN", "DNA", "AA", or NULL
     */
    Alignment(NxsDataBlock *data_block, char *sequence_type, const string& model);

    /**
            destructor
     */
    virtual ~Alignment();


    /****************************************************************************
            input alignment reader
     ****************************************************************************/

      /**
               add a pattern into the alignment
               @param pat the pattern
               @param site the site index of the pattern from the alignment
               @param freq frequency of pattern
               @return TRUE if this pattern hadn't already been seen.
        */
    
    bool addPatternLazy(Pattern &pat, intptr_t site, int freq, bool& gaps_only);
    
    /**
            add a pattern into the alignment
            @param pat the pattern
            @param site the site index of the pattern from the alignment
            @param freq frequency of pattern
            @return TRUE if pattern contains only gaps or unknown char. 
     */
    bool addPattern(Pattern &pat, int site, int freq = 1);

    /**
        Update a bunch of patterns that have been added via addPatternLazy
     (by calling
     */

    void updatePatterns(intptr_t oldPatternCount);
    
	/**
		determine if the pattern is constant. update the is_const variable.
	*/
	virtual void computeConst(Pattern &pat);

    void printSiteInfoHeader(ostream& out, const char* filename, bool partition = false);
    /**
        Print all site information to a stream
        @param out output stream
        @param part_id partition ID, negative to omit
    */
    void printSiteInfo(ostream &out, int part_id);

    /**
        Print all site information to a file
        @param filename output file name
    */
    virtual void printSiteInfo(const char* filename);

    /**
     * add const patterns into the alignment
     * @param freq_const_pattern comma-separated list of const pattern frequencies
     */
    void addConstPatterns(char *freq_const_patterns);

    /**
            read the alignment in NEXUS format
            @param filename file name
            @return 1 on success, 0 on failure
     */
    int readNexus(const char* filename);

    bool buildPattern(StrVector &sequences, const char *sequence_type,
                      int nseq, int nsite);

        /** supporting functions for buildPattern */
        void checkSequenceNamesAreCorrect(int nseq, int nsite, 
                                          double seqCheckStart,
                                          const StrVector &sequences);
        int  determineNumberOfStates(SeqType seq_type, 
                                     const StrVector &sequences,
                                     const char* sequence_type);
        void checkDataType(const char* sequence_type, 
                           const StrVector &sequences, bool& nt2aa);


    bool constructPatterns(int nseq, int nsite,
                           const StrVector& sequences,
                           progress_display_ptr progress);

    /**
            read the alignment in PHYLIP format (interleaved)
            @param filename file name
            @param sequence_type type of the sequence, either "BIN", "DNA", "AA", or NULL
            @return 1 on success, 0 on failure
     */
    int readPhylip(const char *filename, const char *sequence_type);

    /**
            read the alignment in sequential PHYLIP format
            @param filename file name
            @param sequence_type type of the sequence, either "BIN", "DNA", "AA", or NULL
            @return 1 on success, 0 on failure
     */
    int readPhylipSequential(const char *filename, const char *sequence_type);

    /**
            read the alignment in FASTA format
            @param filename file name
            @param sequence_type type of the sequence, either "BIN", "DNA", "AA", or NULL
            @return 1 on success, 0 on failure
     */
    int readFasta(const char *filename, const char *sequence_type);

    /** 
     * Read the alignment in counts format (PoMo).
     *
     * TODO: Allow noninformative sites (where no base is present).
     * 
     * @param filename file name
     * @param sequence_type sequence type (i.e., "CF10")
     *
     * @return 1 on success, 0 on failure
     */
    int readCountsFormat(const char *filename, const char *sequence_type);
        //Supporting functions
        void checkForCustomVirtualPopulationSize(const std::string& model_name, int& N);
        void checkForCustomSamplingMethod(const std::string& model_name, int& N);

    /**
            read the alignment in CLUSTAL format
            @param filename file name
            @param sequence_type type of the sequence, either "BIN", "DNA", "AA", or NULL
            @return 1 on success, 0 on failure
     */
    int readClustal(const char *filename, const char *sequence_type);

    /**
            read the alignment in MSF format
            @param filename file name
            @param sequence_type type of the sequence, either "BIN", "DNA", "AA", or NULL
            @return 1 on success, 0 on failure
     */
    int readMSF(const char *filename, const char *sequence_type);

    /**
            extract the alignment from a nexus data block, called by readNexus()
            @param data_block data block of nexus file
     */
    void extractDataBlock(NxsCharactersBlock *data_block);
        //Supporting function
        void determineSeqTypeStatesAndSymbols
                (NxsCharactersBlock::DataTypesEnum data_type, 
                 NxsCharactersBlock *data_block, char*& symbols);
        void extractStateMatricesFromDataBlock
                (NxsCharactersBlock *data_block,
                 char* char_to_state, char* state_to_char);
        void extractSequenceNamesFromDataBlock
                (NxsCharactersBlock *data_block);

    vector<Pattern> ordered_pattern;
    
    /** lower bound of sum parsimony scores for remaining pattern in ordered_pattern */
    UINT *pars_lower_bound;

    /** order pattern by number of character states and return in ptn_order
        @param pat_type either PAT_INFORMATIVE or 0
    */
    virtual void orderPatternByNumChars(int pat_type);

    /**
     * un-group site-patterns, i.e., making #sites = #patterns and pattern frequency = 1 for all patterns
     */
    void ungroupSitePattern();

    /**
     * re-group site-patterns
     * @param groups number of groups
     * @param site_group group ID (0, 1, ...ngroups-1; must be continuous) of all sites
     */
    void regroupSitePattern(int groups, IntVector &site_group);

    /****************************************************************************
            output alignment 
     ****************************************************************************/

    class CharacterCountsByType {
        public:
            size_t num_nuc;
            size_t num_ungap;
            size_t num_bin;
            size_t num_alpha;
            size_t num_digit;
            CharacterCountsByType();
            void countCharactersByType(StrVector& sequences);
    };

    SeqType detectSequenceType(StrVector &sequences);

    void computeUnknownState();

    void buildStateMap(char *map, SeqType seq_type);

    virtual StateType convertState(char state, SeqType seq_type) const;
    virtual StateType convertBinaryState       (char state) const;
    virtual StateType convertDNAState          (char state) const;
    virtual StateType convertProteinState      (char state) const;
    virtual StateType convertMorphologicalState(char state) const;

    /** 
     * convert state if the number of states (num_states is known)
     * @param state input char to convert
     * @return output char from 0 to 0-num_states or STATE_INVALID or STATE_UNKNOWN
     */
    StateType convertState(char state);

    //virtual void convertStateStr(string &str, SeqType seq_type);

	/**
	 * convert from internal state to user-readable state (e.g., to ACGT for DNA)
	 * Note: does not work for codon data
	 * @param state internal state code
	 * @return user-readable state
	 */
    char convertStateBack(char state) const;

    /** Supporting member functions per specific sequence types */
    char convertBinaryStateBack       (char state) const;
    char convertDNAStateBack          (char state) const;
    char convertProteinStateBack      (char state) const;
    char convertMorphologicalStateBack(char state) const;

    /**
	 * convert from internal state to user-readable state (e.g., to ACGT for DNA)
	 * Note: work for all data
	 * @param state internal state code
	 * @return user-readable state string
	 */
	string convertStateBackStr(StateType state) const;

	/**
            get alignment site range from the residue range relative to a sequence
            @param seq_id reference sequence
            @param residue_left (IN/OUT) left of range
            @param residue_right (IN/OUT) right of range [left,right)
            @return TRUE if success, FALSE if out of range
     */
    bool getSiteFromResidue(int seq_id, int &residue_left, int &residue_right) const;

    int  buildRetainingSites(const char *aln_site_list, 
                             IntVector &kept_sites, int exclude_sites, 
                             const char *ref_seq_name) const;

    void printAlignment(InputType format, const char *filename, bool append = false, 
                        const char *aln_site_list = nullptr, int exclude_sites = 0, 
                        const char *ref_seq_name = nullptr, bool report_progress = true);

    virtual void printAlignment(InputType format, ostream &out, const char* file_name,
                                bool append = false, const char *aln_site_list = nullptr,
                                int exclude_sites = 0, const char *ref_seq_name = nullptr,
                                bool report_progress = true) const;

    void printPhylip(ostream &out, bool append = false, const char *aln_site_list = nullptr,
                     int exclude_sites = 0, const char *ref_seq_name = nullptr, 
                     bool print_taxid = false, bool report_progress = true) const;
    
    void getAllSequences(const char* task_description, StrVector& seq_data) const;
    void getStateStrings(StrVector& stateStrings) const;
    void getOneSequence(const StrVector& stateStrings, size_t seq_id, string& str) const;

    void printFasta(ostream &out, bool append = false, const char *aln_site_list = nullptr,
                    int exclude_sites = 0, const char *ref_seq_name = nullptr,
                    bool report_progress = true) const;

    void printNexus(ostream &out, bool append = false, const char *aln_site_list = NULL,
                    int exclude_sites = 0, const char *ref_seq_name = NULL, 
                    bool print_taxid = false, bool report_progress = true) const;
    /**
            Print the number of gaps per site
            @param filename output file name
     */
    void printSiteGaps(const char *filename);

    /****************************************************************************
            get general information from alignment
     ****************************************************************************/

    /**
            @return number of sequences
     */
    inline intptr_t getNSeq() const {
        return seq_names.size();
    }

    inline int getNSeq32() const {
        return static_cast<int>(seq_names.size());
    }

    /**
            @return number of sites (alignment columns)
     */
    inline size_t getNSite() const {
        return site_pattern.size();
    }

    /**
            @return number of sites (alignment columns);
            for client code that assumes that there are 2^31 sites.
     */
    inline int getNSite32() const {
        return static_cast<int>(site_pattern.size());
    }

    /**
             @return number of patterns
     */
    inline intptr_t getNPattern() const {
        return size();
    }

    inline int getPatternID(size_t site) {
        return site_pattern[site];
    }

    inline Pattern& getPattern(size_t site) {
        return at(site_pattern[site]);
    }

    inline const Pattern& getPattern(size_t site) const {
        return at(site_pattern[site]);
    }

    /**
     * @param out_pattern_index (OUT) vector of size = alignment length storing pattern index of all sites
     */
    virtual void getSitePatternIndex(IntVector &out_pattern_index) {
        out_pattern_index = site_pattern;
    }

    /**
     * @param freq (OUT) vector of site-pattern frequencies
     */
    virtual void getPatternFreq(IntVector &freq);

    /**
     * @param[out] freq vector of site-pattern frequencies
     */
    virtual void getPatternFreq(int *freq);

    /**
            @param i sequence index
            @return sequence name
     */
    const string &getSeqName(intptr_t i) const;
    
    /**
     * Change the name of a sequence
            @param i sequence index
            @param name_to_use name to set it to (it is assumed that
     *            it does not duplicate the name of another sequence; it
     *            is up to the caller to ensure that it does not).
     */
    void  setSeqName(intptr_t i, const string& name_to_use);

    /**
     *  Get a vector containing all the sequence names
     *  @return vector containing the sequence names
     */
    const StrVector& getSeqNames() const;

    /**
    *  populate map from name to number
     * @return number of entries added to the map
     */
    intptr_t  getMapFromNameToID(NameToIDMap& answer) const;

    /**
            @param seq_name sequence name
            @return corresponding ID, -1 if not found
     */
    intptr_t getSeqID(const string &seq_name) const;

    /**
            @return length of the longest sequence name
     */
    size_t getMaxSeqNameLength() const;

    /**
            @param i sequence index
            @return sequence subset number
     */

    int  getSequenceSubset(intptr_t i) const;

    /**
     * Change which subset a sequence is in
     *      @param i sequence index
     *      @param set_no the number of the subset it is to be in.
     */

    void setSequenceSubset(intptr_t i, intptr_t set_no);

    /*
        check if some state is absent, which may cause numerical issues
        @param msg additional message into the warning
        @return number of absent states in the alignment
    */
    virtual int checkAbsentStates(const string& msg);

    /**
            check proper and undupplicated sequence names
     */
    void checkSeqName();
        //Supporting functions
        void renameSequencesIfNeedBe();
        void checkSequenceNamesAreDistinct();
        SequenceInfo* calculateSequenceInfo(const AlignmentSummary* s,
                                            const double* state_freq,
                                            const unsigned* count_per_seq,
                                            int     degrees_of_freedom,
                                            size_t &r_num_problem_seq,
                                            size_t &r_total_gaps, 
                                            size_t &r_num_failed);
        void   reportSequenceInfo (const SequenceInfo* seqInfo, 
                                   size_t max_len) const;
        size_t countGapsInSequence(const AlignmentSummary* s, 
                                   char  firstUnknownState, 
                                   int   seq_index) const;
        void   forgetSequenceInfo (SequenceInfo*& seqInfo) const;

    /**
     * check identical sequences
     * @return the number of sequences that are identical to one of the sequences
     */
    int checkIdenticalSeq();

    /**
     * remove identical sequences from alignment
     * @param not_remove name of sequence where removal is avoided
     * @param keep_two TRUE to keep 2 out of k identical sequences, false to keep only 1
     * @param removed_seqs (OUT) name of removed sequences
     * @param target_seqs (OUT) corresponding name of kept sequence that is identical to the removed sequences
     * @return this if no sequences were removed, or new alignment if at least 1 sequence was removed
     */
    virtual Alignment* removeIdenticalSeq(string not_remove, bool keep_two, 
                                          StrVector &removed_seqs, 
                                          StrVector &target_seqs);

        //Supporting functions for removeIdenticalSeq...
        bool shouldRemoveSequence(intptr_t seq1, intptr_t seq2, 
                                  const string& not_remove ,
                                  const BoolVector& isSequenceRemoved,
                                  const vector<size_t>& hashes) const;

        void reportSequenceKept(intptr_t seq1, intptr_t seq2, bool listIdentical,
                                progress_display& progress) const;

        void doneCheckingForDuplicateSequences(double startCheck, 
                                            progress_display& progress) const;

        Alignment* removeSpecifiedSequences(const StrVector&  removed_seqs,
                                            const BoolVector& isSequenceRemoved);
    
    /**
     * calclulate hashes of all the sequences (note: hashes depend on patterns, so
     * although hashes are valid for comparing sequences in the same Alignment,
     * they are not valid for comparing sequences between two Alignment instances
     * (use the slower getPatternIndependentSequenceHashes() for that).
     * @param progress - progress_display instance against which progress is to be reported
     * @return a vector ( vector<size_t> ) of the hashes of all the sequences
     */
    vector<size_t> getSequenceHashes(progress_display_ptr progress) const;

    /**
     * calclulate hashes of all the sequences (these hashes are independent of the patterns,
     * and can be used to compare sequences from different Alignment instances).
     * @param progress - progress_display instance against which progress is to be reported
     * @return a vector ( vector<size_t> ) of the hashes of all the sequences
     */

    vector<size_t> getPatternIndependentSequenceHashes(progress_display_ptr progress) const;

    /**
     * calculating hashes for sequences
     * @param v state at a given site, in the sequence being hashed
     * @param hash running hash value for the sequence (modified)
     */
    void adjustHash(StateType v, size_t& hash) const;
    void adjustHash(bool      v, size_t& hash) const;
    
    /**
            Quit if some sequences contain only gaps or missing data
     */
	virtual void checkGappySeq(bool force_error = true);

	/**
	 * return a new alignment if some sequence is totally gappy, or this if all sequence are okey
	 */
	Alignment *removeGappySeq();

    /**
            @return TRUE if seq_id contains only gaps or missing characters
            @param seq_id sequence ID
     */
    bool isGapOnlySeq(intptr_t seq_id);

    virtual bool isSuperAlignment() {
        return false;
    }

    /****************************************************************************
            alignment general processing
     ****************************************************************************/

    /**
            extract sub-alignment of a sub-set of sequences
            @param aln original input alignment
            @param seq_id ID of sequences to extract from
            @param min_true_cher the minimum number of non-gap characters, true_char<min_true_char -> delete the sequence
            @param min_taxa only keep alignment that has >= min_taxa sequences
            @param[out] kept_partitions (for SuperAlignment) indices of kept partitions
     */
    virtual void extractSubAlignment(Alignment *aln, IntVector &seq_id, int min_true_char, int min_taxa = 0, IntVector *kept_partitions = NULL);

    /**
            extract a sub-set of patterns
            @param aln original input alignment
            @param ptn_id ID of patterns to extract from
     */
    void extractPatterns(Alignment *aln, IntVector &ptn_id);

    /**
            extract a sub-set of patterns
            @param aln original input alignment
            @param ptn_freq pattern frequency to extract from
     */
    void extractPatternFreqs(Alignment *aln, IntVector &ptn_freq);

    /**
            create a non-parametric bootstrap alignment from an input alignment
            @param aln input alignment
            @param pattern_freq (OUT) resampled pattern frequencies if not NULL
            @param spec bootstrap specification of the form "l1:b1,l2:b2,...,lk:bk"
            	to randomly draw b1 sites from the first l1 sites, etc. Note that l1+l2+...+lk
            	must equal m, where m is the alignment length. Otherwise, an error will occur.
            	If spec == NULL, a standard procedure is applied, i.e., randomly draw m sites.
     */
    virtual void createBootstrapAlignment(Alignment *aln, IntVector* pattern_freq = nullptr, 
                                          const char *spec = nullptr);

    /**
            resampling pattern frequency by a non-parametric bootstrap 
            @param pattern_freq (OUT) resampled pattern frequencies
            @param spec bootstrap specification, see above
     */
    virtual void createBootstrapAlignment(IntVector &pattern_freq, 
                                          const char *spec = nullptr);

    /**
            resampling pattern frequency by a non-parametric bootstrap
            @param pattern_freq (OUT) resampled pattern frequencies
            @param spec bootstrap specification, see above
            @param rstream random generator stream, NULL to use the global randstream
     */
    virtual void createBootstrapAlignment(int *pattern_freq, const char *spec = nullptr, 
                                          int *rstream = nullptr);

	/**
			Diep: This is for UFBoot2-Corr
			Initialize "this" alignment as a bootstrap alignment
			@param aln: the reference to the original alignment
			@new_pattern_freqs: the frequencies of patterns to be present in bootstrap aln
	 */
	void buildFromPatternFreq(Alignment & aln, IntVector new_pattern_freqs);

    /**
            create a gap masked alignment from an input alignment. Gap patterns of masked_aln 
                    will be superimposed into aln to create the current alignment object.
            @param aln input alignment
            @param masked_aln gappy alignment of the same size with aln
     */
    void createGapMaskedAlignment(Alignment *masked_aln, Alignment *aln);

    /**
	 * shuffle alignment by randomizing the order of sites
	 */
	virtual void shuffleAlignment();

	/**
            concatenate an alignment into the current alignment object
            @param aln an alignment of the same number of sequences and sequence names    
     */
    void concatenateAlignment(Alignment *aln);

    /**
            copy the input alignment into the current alignment object
            @param aln input alignment
     */
    void copyAlignment(Alignment *aln);
    
    void copyStateInfoFrom(const Alignment* aln);
    
    /** Update this alignment, from another
     *  @param other - the source alignment (may not be == this)
     *  @param updated_sequences - vector indicating sequences to be replaced
     *  (in each pair, first is the id in this alignment, second is the id in the other alignment)
     *  @param added_sequences - vector indicating sequences to be added
     *  (ids of those sequences in the other alignment)
     */
    bool updateFrom(const Alignment* other,
                    const std::vector<std::pair<int,int>>& updated_sequences,
                    const IntVector& added_sequences,
                    progress_display_ptr progress);
    
    /** Check if this alignment can be updated from another (called from: updateFrom())
     *  @param other - the source alignment (may not be == this)
     *  @return true if the other alignment is compatible
     */
    bool isCompatible(const Alignment* other, std::string& whyNot) const;

    /**
            extract a sub-set of sites
            @param aln original input alignment
            @param ptn_id ID of sites to extract from (starting from 0)
     */
    void extractSites(Alignment *aln, IntVector &site_id);

    /**
            extract a sub-set of sites
            @param aln original input alignment
            @param spec specification of positions, e.g. "1-100,101-200\2"
     */
    void extractSites(Alignment *aln, const char* spec);

    /**
        convert a DNA alignment into codon or AA alignment
    */
    void convertToCodonOrAA(Alignment *aln, char *gene_code_id, bool nt2aa = false);

        /** supporting functions for convertToCodonOrAA */
        void convertSiteToCodonOrAA(Alignment* aln,  bool nt2aa,
                                    const char* AA_to_state,
                                    size_t      site, Pattern& pat,
                                    int&        num_error,
                                    std::ostringstream& err_str);
        void reportIfStateInvalid  (size_t site, size_t seq,
                                    char state, int&   num_error,
                                    std::ostringstream& err_str);


    /**
     convert this codon alignment to AA
     */
    Alignment *convertCodonToAA();

    /**
     convert this codon alignment to DNA
     */
    Alignment *convertCodonToDNA();

    /**
     @param quartet ID of four taxa
     @param[out] support number of sites supporting 12|34, 13|24 and 14|23
     */
    virtual void computeQuartetSupports(IntVector &quartet, vector<int64_t> &support);
    
    /****************************************************************************
            Distance functions
     ****************************************************************************/

    /**
            compute the observed distance (number of different pairs of positions per site) 
                    between two sequences
            @param seq1 index of sequence 1
            @param seq2 index of sequence 2
            @return the observed distance between seq1 and seq2 (between 0.0 and 1.0)
     */
    virtual double computeObsDist(int seq1, int seq2) const;

    /**
            @param obs_dist the observed distance between two sequences
            @return Jukes-Cantor corrected distance between those sequences
     */
    double computeJCDistanceFromObservedDistance(double obs_dist) const;
    
    /**
            @param seq1 index of sequence 1
            @param seq2 index of sequence 2
            @return Jukes-Cantor correction distance between seq1 and seq2
     */
    double computeJCDist(int seq1, int seq2) const;

    /**
            abstract function to compute the distance between 2 sequences. The default return
            Juke-Cantor corrected distance.
            @param seq1 index of sequence 1
            @param seq2 index of sequence 2		
            @return any distance between seq1 and seq2
     */
    virtual double computeDist(int seq1, int seq2) const {
        return computeJCDist(seq1, seq2);
    }

    /**
            write distance matrix into a file in PHYLIP distance format
            @param format       distance file format (starting "upper", "lower", or "square"
                            (indicating upper/lower triangular or square matrix format)
            @param file_name distance file name
            @param dist_mat distance matrix
     */
    void printDist(const std::string& format, int compression_level,
                   const char *file_name, double *dist_mat) const;

    /**
            write distance matrix into a stream in PHYLIP distance format
            @param out output stream
            @param dist_mat distance matrix
     */
    template <class S> void printDist(const std::string&, S &out,
                                      double *dist_mat) const;

    /**
            read distance matrix from a file in PHYLIP distance format
            @param file_name distance file name
            @param dist_mat distance matrix
            @return the longest distance
     */
    double readDist(const char *file_name, bool is_incremental, double *dist_mat);

    /**
            read distance matrix from a stream in PHYLIP distance format
            @param in input stream
            @param dist_mat distance matrix
     */
    double readDist(igzstream &in, bool is_incremental, double *dist_mat);


    /****************************************************************************
            some statistics
     ****************************************************************************/

    /**
        count occurrences for each state from 0 to STATE_UNKNOWN
        @param startSite ordinal of first site (assumed 0 and <= stopSite)
        @param stopSite   ordinal of last site (assumed +ve and <= size())
        @param[out] state_count counts for all states (for a subset of sites)
     */
    void countStatesForSites(size_t startSite, size_t stopSite, 
                             size_t *state_count) const;
    
    /**
        count occurrences for each state from 0 to STATE_UNKNOWN
        @param[out] state_count counts for all states
        @param num_unknown_states number of unknown states e.g. for missing data
     */
    void countStates(size_t *state_count, 
                     size_t num_unknown_states) const;

    void countStatesForSubset(const IntVector& subset,
                              std::vector<size_t>& state_count) const;
    
    /**
        convert counts to frequencies using EM algorithm
        @param[in] state_count counts for all states
        @paramp[out] state_freq normalized state frequency vector
     */
    void convertCountToFreq(size_t *state_count, double *state_freq) const;

    /**
            compute empirical state frequencies from the alignment
            @param state_freq (OUT) is filled with state frequencies, assuming state_freq was allocated with 
                    at least num_states entries.
            @param num_unknown_states number of unknown states e.g. for missing data
            @param report_to_tree (the tree, if any, corresponding to the alignment,
                    for which state frequencies are being calculated)
     */
    virtual void computeStateFreq(double *state_freq,
                                  size_t num_unknown_states,
                                  PhyloTree* report_to_tree);

    /**
            compute empirical state frequencies, for one subset,
            from the alignment
            @param taxon_subset The ids of the taxa in the subset
            @param state_freq (OUT) is filled with state frequencies, assuming state_freq was allocated with 
                    at least num_states entries.
            @param report_to_tree (the tree, if any, corresponding to the alignment,
                    for which state frequencies are being calculated)
     */
    virtual void computeStateFreqForSubset(const IntVector& taxon_subset,
                                           double* state_freq) const;

    int convertPomoState(int state) const;

    /** 
     * Compute the absolute frequencies of the different states.
     * Helpful for models with many states (e.g., PoMo) to check the
     * abundancy of states in the data.
     * 
     * @param abs_state_freq (OUT) assumed to be at least of size
     * num_states.
     */
    void computeAbsoluteStateFreq(unsigned int *abs_state_freq);
    
    /**
            compute empirical state frequencies for each sequence 
            @param freq_per_sequence (OUT) state frequencies for each sequence, of size num_states*num_freq
     */
    void computeStateFreqPerSequence (double *freq_per_sequence);

    void countStatePerSequence (unsigned *count_per_sequence);

    /**
     * Make all frequencies a little different and non-zero
     * @param stateFrqArr (IN/OUT) state frequencies
     */
    void convfreq(double *stateFrqArr) const;

    /**
	 * compute special empirical frequencies for codon alignment: 1x4, 3x4, 3x4C
	 * @param state_freq (OUT) is filled with state frequencies, assuming state_freq was allocated with
	 * at least num_states entries.
	 * @param freq either FREQ_CODON_1x4, FREQ_CODON_3x4, or FREQ_CODON_3x4C
	 * @param ntfreq (OUT) nucleotide frequencies, assuming of size 4 for F1x4 and of size 12 for F3x4.
	 */
	void computeCodonFreq(StateFreqType freq, double *state_freq, double *ntfreq);

        /** supporting functions for calculating codon frequences */
        void computeCodonFreq_1x4(double *state_freq, double *ntfreq);
        void computeCodonFreq_3x4(double *state_freq, double *ntfreq);
        void computeEmpiricalFrequencies(double *state_freq);

	/**
            compute empirical substitution counts between state pairs
            @param normalize true to normalize row sum to 1, false otherwise
            @param[out] pair_freq matrix of size num_states*num_states
            @param[out] state_freq vector of size num_states
     */
    virtual void computeDivergenceMatrix(double* pair_freq, double* state_freq, 
                                         bool    normalize);

    /**
        perform matched-pair tests of symmetry of Lars Jermiin et al.
        @param[out] sym results of test of symmetry
        @param[out] marsym results of test of marginal symmetry
        @param[out] intsym results of test of internal symmetry
        @param out output stream to print results
        @param rstream random stream to shuffle alignment columns
        @param out_stat output stream to print pairwise statistics
     */
    virtual void doSymTest(size_t vecid, vector<SymTestResult> &sym, vector<SymTestResult> &marsym,
                           vector<SymTestResult> &intsym, int *rstream = NULL, vector<SymTestStat> *stats = NULL);

    /**
            count the fraction of constant sites in the alignment, update the variable frac_const_sites
     */
    virtual void countConstSite();

    /**
     * generate uninformative patterns
     */
    void generateUninfPatterns(StateType repeat, vector<StateType> &singleton, vector<int> &seq_pos, vector<Pattern> &unobserved_ptns);
        
    /**
     * @param missing_data TRUE for missing data aware correction (for Mark Holder)
     * @param[out] unobserved_ptns unobserved constant patterns, each entry encoding for one constant character
     */
    void getUnobservedConstPatterns(ASCType ASC_type, vector<Pattern> &unobserved_ptns);
        //Supporting functions
        void getUnobservedConstPatternsLewis(vector<Pattern> &unobserved_ptns);
        void getUnobservedConstPatternsHolder(vector<Pattern> &unobserved_ptns);
        void getUnobservedConstPatternsHolderForInformativeSites(vector<Pattern> &unobserved_ptns);

    /**
            @return the number of ungappy and unambiguous characters from a sequence
            @param seq_id sequence ID
     */
    int countProperChar(int seq_id) const;

    /**
            @return unconstrained log-likelihood (without a tree)
     */
    virtual double computeUnconstrainedLogL() const;

    /**
     * 	@return number of states, if it is a partition model, return max num_states across all partitions
     */
    virtual int getMaxNumStates() const { return num_states; }

    /** either SEQ_BINARY, SEQ_DNA, SEQ_PROTEIN, SEQ_MORPH, or SEQ_CODON */
    SeqType seq_type;

    StateType STATE_UNKNOWN;

    /**
            fraction of constant sites
     */
    double frac_const_sites;
    
    /**
            fraction of invariant sites, incl. const sites and site like G-S-GG-GGGG
     */
    double frac_invariant_sites;

    /** number of parsimony informative sites */
    int num_informative_sites;

    /** number of variant sites */
    int num_variant_sites;

    /** number of sites used for parsimony computation, can be informative or variant */
    int num_parsimony_sites;

	/**
	 *  map from 64 codon to non-stop codon index
	 */
    IntVector non_stop_codon;

	/**
	 * For codon sequences: index of 61 non-stop codons to 64 codons
	 * For other sequences: empty
	 */
	IntVector codon_table;

	/**
	 * For codon_sequences: 64 amino-acid letters for genetic code of AAA,AAC,AAG,AAT,...,TTT
	 * For other sequences: empty
	 */
	std::string genetic_code;

	/**
	 * Virtual population size for PoMo model
	 */
	int virtual_pop_size;

  // TODO DS: Maybe change default to SAMPLING_WEIGHTED_HYPER.
  /// The sampling method (defaults to SAMPLING_WEIGHTED_BINOM).
  SamplingType pomo_sampling_method;

  /** BQM: 2015-07-06, 
      for PoMo data: map from state ID to pair of base1 and base2 
      represented in the high 16-bit and the low 16-bit of uint32_t
      for base1, bit0-1 is used to encode the base (A,G,C,T) and the remaining 14 bits store the count
      same interpretation for base2
  */
  vector<uint32_t> pomo_sampled_states;
  IntIntMap pomo_sampled_states_index; // indexing, to quickly find if a PoMo-2-state is already present

    /* for site-specific state frequency model with Huaichun, Edward, Andrew */
    
    /* site to model ID map */
    IntVector site_model;
    
    /** site to state frequency vector */
    vector<double*> site_state_freq;

    /** vector counting the number of singleton parsimony states
        for each taxon (used in Parsimony Joining).*/
    std::vector<UINT> singleton_parsimony_states;

    UINT total_singleton_parsimony_states;
    
    /**
     * @return true if data type is SEQ_CODON and state is a stop codon
     */
    bool isStopCodon(int state);

    bool isStandardGeneticCode();

	/**
	 * @return number of non-stop codons in the genetic code
	 */
	int getNumNonstopCodons();

    /* build seq_states containing set of states per sequence
     * @param add_unobs_const TRUE to add all unobserved constant states (for +ASC model)
     */
    //virtual void buildSeqStates(vector<vector<int> > &seq_states, bool add_unobs_const = false);

    /** Added by MA
            Compute the probability of this alignment according to the multinomial distribution with parameters determined by the reference alignment
            @param refAlign the reference alignment
            @param prob (OUT) the returned probabilty
		
            The probability is computed as follows:
            - From the reference alignment, we count the relative pattern frequencies p_1 ... p_k (sum = 1)
            - From THIS alignment, we have frequencies d_1 ... d_k (sum = len = nsite)
            - Prob(THIS | refAlign) = nsite!/(d_1! * ... * d_k!) product(p_i^d_i)
     */
    void multinomialProb(Alignment refAlign, double &prob);

    /** Added by MA
            Compute the probability of the `expected alignment' according to the multinomial distribution with parameters determined by the pattern's observed frequencies in THIS alignment.
            The `expected alignment' consists of patterns with log-likelihoods (under some model+tree) given in the input file (logLL).
            Note that order of the log-likelihoods in inputLL must corresponds to patterns in THIS alignment.

            @param inputLL the input patterns log-likelihood vector
            @param prob (OUT) the returned probability
     */
    void multinomialProb(DoubleVector logLL, double &prob);
    void multinomialProb(double *logLL, double &prob);

    /** Adapted from MA
            compute the probability of the alignment defined by pattern_freq given this alignment	
     */
    double multinomialProb(IntVector &pattern_freq);


    /**
            get the appearance for a state, helpful for ambigious states

            For nucleotides, the appearances of A, and C are 1000 and 0100,
            respectively. If a state is ambiguous, more than one 1 will show up.
            The appearance of the unknown state is 1111.

            @param state the state index
            @param state_app (OUT) state appearance
     */
    void getAppearance(StateType state, double *state_app) const;

    void getAppearance(StateType state, StateBitset &state_app) const;

    UINT getCountOfSingletonParsimonyStates() const;

	/**
	 * read site specific state frequency vectors from a file to create corresponding model
     * update site_model and site_state_freq variables for this class
	 * @param aln input alignment
	 * @param site_freq_file file name
     * @return TRUE if alignment needs to be changed, FALSE otherwise
	 */
	bool readSiteStateFreq(const char* site_freq_file);

        void readSiteStateFreqFromFile
                (ifstream& in, const IntVector& pattern_to_site,
                 bool& aln_changed);
        void checkForEqualityOfSites
                (const IntVector& pattern_to_site,
                 const IntVector& site_id, 
                 double *site_freq_entry, bool& aln_changed);
                //note: Takes ownership of site_freq_entry,
                //      which should have been allocated with
                //      operator new [].
        void handleUnspecifiedSites
                (int specified_sites, bool& aln_changed);
    /**
     * disable the display of progress bars for actions being carried out by this alignment
     */
    void showNoProgress();

protected:

    /**
            sequence names
     */
    StrVector seq_names;

    /**
            subsets, into which sequences are to go, when a 
            divergent model is in use (or when subsets are
            being used to speed up initial tree construction)
     */
    StrVector subset_names;

    /**
            maps, each sequence (same index as seq_names) to
            a subset (note: subsets aren't necessarily 
            subtrees)
     */
    IntVector seq_to_subset;

    /**
            Site to pattern index
     */
    IntVector site_pattern;
    

    /**
            hash map from pattern to index in the vector of patterns (the alignment)
     */
    PatternIntMap pattern_index;

    bool isShowingProgressDisabled;


    void readAlignmentFile(InputType intype, const char* filename,
                           const char* requested_sequence_type);

    /**
	 * special initialization for codon sequences, e.g., setting #states, genetic_code
	 * @param sequence_type user-defined sequence type
	 */
	void initCodon           (const char *gene_code_id, bool nt2aa);
    void readDistLine        (igzstream &in, intptr_t nseqs, intptr_t seq1,
                              bool &upper, bool &lower, double &longest_dist,
                              double *tmp_dist_mat,
                              std::map<string,intptr_t>& map_seqName_ID,
                              progress_display& readProgress);
    void readShortDistLine   (const std::string& seq_name,
                              intptr_t seq1, intptr_t seq2, intptr_t rowStop,
                              bool& upper, bool& lower, double *tmp_dist_mat,
                              progress_display& readProgress);
    void mapLoadedSequencesToAlignment(std::map<string,intptr_t>& map_seqName_ID,
                                       intptr_t nseqs, bool is_incremental, 
                                       intptr_t* actualToTemp);
    void copyToDistanceMatrix     (double* tmp_dist_mat, intptr_t nseqs,
                                   intptr_t* actualToTemp, double* dist_mat);
    void checkForSymmetricMatrix  (double* dist_mat, intptr_t nseqs); 

    void readFirstLineOfPhylipFile(const std::string& line,
                                   int& nseq, int& nsite);
    void readFastaSequenceData    (igzstream& in, StrVector& sequences);
    void parseMSFSequenceNameLine (std::string line, int line_num,
                                   StrVector&  sequences, int& seq_len);

};


void extractSiteID(Alignment *aln, const char* spec, IntVector &site_id);

/**
 create a new Alignment object with possibility of comma-separated file names
 @param aln_file alignment file name, can be a comma-separated list of file names
 @param sequence_type sequence data type
 @param input input file format
 @param model_name model name
 */
Alignment *createAlignment(string aln_file, const char *sequence_type, InputType intype, string model_name);

#endif
