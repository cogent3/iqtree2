//
// modelinfo.cpp
// Created by James Barbetti on 28-Jan-2021
//

#include "modelinfo.h"
#include "modelsubst.h"      //for OPEN_BRACKET and CLOSE_BRACKET
#include "modelfileloader.h"
#include "modelexpression.h" //for InterpretedExpression

#include "modeldna.h"        //for ModelDNA
#include "modeldnaerror.h"   //for ModelDNAError
#include "modelprotein.h"    //for ModelProtein

#include <utils/my_assert.h> //for ASSERT macro
#include <utils/stringfunctions.h> //for convert_int
#include <utils/tools.h>     //for outError

VerboseMode YAMLModelVerbosity = VB_MIN;

ModelInfoFromName::ModelInfoFromName(std::string name): model_name(name) {}
ModelInfoFromName::ModelInfoFromName(const char* name): model_name(name) {}

namespace {
    bool startsWith(std::string s, const char* front) {
        auto frontLen = strlen(front);
        return (s.substr(0, frontLen) == front);
    }

    bool endsWith(const std::string s, const char* suffix) {
        auto suffixLen = strlen(suffix);
        if (s.length() < suffixLen) {
            return false;
        }
        return s.substr(s.length()-suffixLen, suffixLen) == suffix;
    }

    bool contains(std::string s, const char* pattern) {
        return s.find(pattern) != std::string::npos;
    }

    std::string::size_type findSubStr(const std::string &name,
                                      const std::string sub1,
                                      const std::string sub2) {
        std::string::size_type pos1, pos2;
        for (pos1 = 0; pos1 != std::string::npos; pos1++) {
            pos1 = name.find(sub1, pos1);
            if (pos1 == std::string::npos) {
                break;
            }
            if (pos1+2 >= name.length() || !isalpha(name[pos1+2])) {
                break;
            }
        }
        for (pos2 = 0; pos2 != std::string::npos; pos2++) {
            pos2 = name.find(sub2, pos2);
            if (pos2 == std::string::npos) {
                break;
            }
            if (pos2+2 >= name.length() ||!isalpha(name[pos2+2])) {
                break;
            }
        }
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            return (pos1 < pos2) ? pos1 : pos2;
        } else if (pos1 != std::string::npos) {
            return pos1;
        } else {
            return pos2;
        }
    }

    std::string::size_type findSubStr(const std::string &name,
                                      const char * sub1,
                                      const char* sub2) {
        std::string s1(sub1);
        std::string s2(sub2);
        return findSubStr(name, s1, s2);
    }

    std::string::size_type posPOMO(const std::string &model_name) {
        return findSubStr(model_name, "+P", "*P");
    }
};

std::string ModelInfoFromName::getFreeRateParameters
    (int& num_rate_cats, bool& fused_mix_rate) const {
     string::size_type posR = model_name.find("+R"); // FreeRate model
     string::size_type posR2 = model_name.find("*R"); // FreeRate model

    std::string freerate_params;
    if (posR != string::npos && posR2 != string::npos) {
        auto posFirst = (posR < posR2) ? posR : posR2;
        std::cout << "NOTE: both +R and *R were specified, continue with "
                  << model_name.substr(posFirst,2) << std::endl;
    }
    if (posR2 != string::npos && posR2 < posR) {
        posR = posR2;
        fused_mix_rate = true;
    }
    // FreeRate model
    int end_pos = 0;
    if (model_name.length() > posR+2 && isdigit(model_name[posR+2])) {
        num_rate_cats = convert_int(model_name.substr(posR+2).c_str(), end_pos);
        if (num_rate_cats < 1) {
            outError("Wrong number of rate categories");
        }
    }
    if (model_name.length() > posR+2+end_pos &&
        model_name[posR+2+end_pos] == OPEN_BRACKET) {
        auto close_bracket = model_name.find(CLOSE_BRACKET, posR);
        if (close_bracket == string::npos) {
            outError("Close bracket not found in ", model_name);
        }
        auto param_start = posR+3+end_pos;
        auto param_len   = close_bracket-posR-3-end_pos;
        freerate_params  = model_name.substr(param_start, param_len);
    } else if (model_name.length() > posR+2+end_pos &&
               model_name[posR+2+end_pos] != '+') {
        outError("Wrong model name ", model_name);
    }
    return freerate_params;
}

std::string ModelInfoFromName::getFrequencyMixtureParams
    (std::string& freq_str) const {
    // first handle mixture frequency
    std::string::size_type posfreq = model_name.find("+FMIX");
    size_t close_bracket;
    freq_str = model_name;

    if (posfreq != string::npos) {
        string fmix_str;
        size_t last_pos = freq_str.find_first_of("+*", posfreq+1);

        if (last_pos == string::npos) {
            fmix_str = freq_str.substr(posfreq);
            freq_str = freq_str.substr(0, posfreq);
        } else {
            fmix_str = freq_str.substr(posfreq, last_pos-posfreq);
            freq_str = freq_str.substr(0, posfreq)
                     + freq_str.substr(last_pos);
        }
        if (fmix_str[5] != OPEN_BRACKET) {
            outError("Mixture-frequency must start with +FMIX{");
        }
        close_bracket = fmix_str.find(CLOSE_BRACKET);
        if (close_bracket == string::npos) {
            outError("Close bracket not found in ", fmix_str);
        }
        if (close_bracket != fmix_str.length()-1) {
            outError("Wrong close bracket position ", fmix_str);
        }
        return fmix_str.substr(6, close_bracket-6);
    }
    return "";
}

void ModelInfoFromName::getFrequencyOptions(std::string& freq_str,
                                            StateFreqType& freq_type,
                                            std::string& freq_params,
                                            bool& optimize_mixmodel_weight) const {
    // then normal frequency
    freq_str = model_name;
    std::string::size_type posfreq = freq_str.find("+FO");
    if (posfreq== string::npos) {
        posfreq = freq_str.find("+Fo");
        if (posfreq == string::npos) {
            posfreq = freq_str.find("+F");
        }
    }
    if (posfreq != string::npos) {
        string fstr;
        size_t last_pos = freq_str.find_first_of("+*", posfreq+1);
        if (last_pos == string::npos) {
            fstr = freq_str.substr(posfreq);
            freq_str = freq_str.substr(0, posfreq);
        } else {
            fstr = freq_str.substr(posfreq, last_pos-posfreq);
            freq_str = freq_str.substr(0, posfreq)
            + freq_str.substr(last_pos);
        }
        if (fstr.length() > 2 && fstr[2] == OPEN_BRACKET) {
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with user-defined frequency"
                         " is not allowed");
            }
            auto close_bracket = fstr.find(CLOSE_BRACKET);
            if (close_bracket == string::npos) {
                outError("Close bracket not found in ", fstr);
            }
            if (close_bracket != fstr.length()-1) {
                outError("Wrong close bracket position ", fstr);
            }
            freq_type = FREQ_USER_DEFINED;
            freq_params = fstr.substr(3, close_bracket-3);
        } else if (fstr == "+FC" || fstr == "+Fc" || fstr == "+F") {
            if (freq_type == FREQ_MIXTURE) {
                freq_params = "empirical," + freq_params;
                optimize_mixmodel_weight = true;
            } else {
                freq_type = FREQ_EMPIRICAL;
            }
        } else if (fstr == "+FU" || fstr == "+Fu") {
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with user-defined frequency"
                         " is not allowed");
            }
            else {
                freq_type = FREQ_USER_DEFINED;
            }
        } else if (fstr == "+FQ" || fstr == "+Fq") {
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with equal frequency"
                         " is not allowed");
            }
            else {
                freq_type = FREQ_EQUAL;
            }
        } else if (fstr == "+FO" || fstr == "+Fo") {
            if (freq_type == FREQ_MIXTURE) {
                freq_params = "optimize," + freq_params;
                optimize_mixmodel_weight = true;
            } else {
                freq_type = FREQ_ESTIMATE;
            }
        } else if (fstr == "+F1x4" || fstr == "+F1X4") {
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with " + fstr + " is not allowed");
            }
            else {
                freq_type = FREQ_CODON_1x4;
            }
        } else if (fstr == "+F3x4" || fstr == "+F3X4") {
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with " + fstr + " is not allowed");
            }
            else {
                freq_type = FREQ_CODON_3x4;
            }
        } else if (fstr == "+F3x4C" || fstr == "+F3x4c" ||
                   fstr == "+F3X4C" || fstr == "+F3X4c") {
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with " + fstr + " is not allowed");
            }
            else {
                freq_type = FREQ_CODON_3x4C;
            }
        } else if (fstr == "+FRY") {
            // MDW to Minh: I don't know how these should interact with FREQ_MIXTURE,
            // so as nearly everything else treats it as an error, I do too.
            // BQM answer: that's fine
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with " + fstr + " is not allowed");
            }
            else {
                freq_type = FREQ_DNA_RY;
            }
        } else if (fstr == "+FWS") {
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with " + fstr + " is not allowed");
            }
            else {
                freq_type = FREQ_DNA_WS;
            }
        } else if (fstr == "+FMK") {
            if (freq_type == FREQ_MIXTURE) {
                outError("Mixture frequency with " + fstr + " is not allowed");
            }
            else {
                freq_type = FREQ_DNA_MK;
            }
        } else {
            // might be "+F####" where # are digits
            try {
                freq_type = parseStateFreqDigits(fstr.substr(2));
                // throws an error if not in +F#### format
            } catch (...) {
                outError("Unknown state frequency type ",fstr);
            }
        }
        //model_str = model_str.substr(0, posfreq);
    }
}

void ModelInfoFromName::getGammaParameters(int& num_rate_cats,
                                           double& gamma_shape) const {
    string::size_type posG = model_name.find("+G");
    string::size_type posG2 = model_name.find("*G");
    if (posG != string::npos && posG2 != string::npos) {
        auto posFirst = (posG < posG2) ? posG : posG2;
        std::cout << "NOTE: both +G and *G were specified, continue with "
                  << model_name.substr(posFirst,2) << std::endl;
    }
    if (posG2 != string::npos && posG2 < posG) {
        posG = posG2;
    }
    int end_pos = 0;
    if (model_name.length() > posG+2 &&
        isdigit(model_name[posG+2])) {
        auto rest     = model_name.substr(posG+2);
        num_rate_cats = convert_int(rest.c_str(), end_pos);
        if (num_rate_cats < 1) {
            outError("Wrong number of rate categories");
        }
    }
    if (model_name.length() > posG+2+end_pos &&
        model_name[posG+2+end_pos] == OPEN_BRACKET) {
        auto close_bracket = model_name.find(CLOSE_BRACKET, posG);
        if (close_bracket == std::string::npos) {
            outError("Close bracket not found in ", model_name);
        }
        gamma_shape = convert_double(model_name.substr(posG+3+end_pos, close_bracket-posG-3-end_pos).c_str());
        //if (gamma_shape < MIN_GAMMA_SHAPE ||
        //    gamma_shape > MAX_GAMMA_SHAPE) {
        //    stringstream str;
        //    str << "Gamma shape parameter " << gamma_shape << "out of range ["
        //        << MIN_GAMMA_SHAPE << ',' << MAX_GAMMA_SHAPE << "]\n";
        //    outError(str.str());
        //}
    } else if (model_name.length() > posG+2+end_pos &&
               model_name[posG+2+end_pos] != '+') {
        outError("Wrong model name ", model_name);
    }
}

std::string ModelInfoFromName::getHeterotachyParameters
    (bool is_mixture_model, int& num_rate_cats,
     bool& fused_mix_rate) const {
    string::size_type posH  = model_name.find("+H"); // heterotachy model
    string::size_type posH2 = model_name.find("*H"); // heterotachy model

    if (posH != string::npos && posH2 != string::npos) {
        auto posFirst = (posH < posH2) ? posH : posH2;
        std::cout << "NOTE: both +H and *H were specified, continue with "
                  << model_name.substr(posFirst,2) << std::endl;
    }
    if (posH2 != string::npos && posH2 < posH) {
        posH = posH2;
        fused_mix_rate = true;
    }
    std::string heterotachy_params;
    // Heterotachy model
    int end_pos = 0;
    if (model_name.length() > posH+2 && isdigit(model_name[posH+2])) {
        auto rest = model_name.substr(posH+2);
        num_rate_cats = convert_int(rest.c_str(), end_pos);
        if (num_rate_cats < 1) {
            outError("Wrong number of rate categories");
        }
    } else if (!is_mixture_model || !fused_mix_rate) {
        outError("Please specify number of heterotachy classes (e.g., +H2)");
    }
    if (model_name.length() > posH+2+end_pos &&
        model_name[posH+2+end_pos] == OPEN_BRACKET) {
        auto close_bracket = model_name.find(CLOSE_BRACKET, posH);
        if (close_bracket == string::npos) {
            outError("Close bracket not found in ", model_name);
        }
        auto hetero_start  = posH+3+end_pos;
        auto hetero_len    = close_bracket-posH-3-end_pos;
        heterotachy_params = model_name.substr(hetero_start, hetero_len);
    } else if (model_name.length() > posH+2+end_pos &&
               model_name[posH+2+end_pos] != '+') {
        outError("Wrong model name ", model_name);
    }
    return heterotachy_params;
}

double ModelInfoFromName::getProportionOfInvariantSites() const {
    string::size_type posI = model_name.find("+I");
    if (posI == string::npos) {
        outError("Cannot determine proportion of invariant sites"
                 " for model ", model_name);
    }
    // invariant site model
    if (model_name.length() > posI+2 &&
        model_name[posI+2] == OPEN_BRACKET) {
        auto close_bracket = model_name.find(CLOSE_BRACKET, posI);
        if (close_bracket == string::npos) {
            outError("Close bracket not found in ", model_name);
        }
        std::string num = model_name.substr(posI+3, close_bracket-posI-3);
        double p_invar_sites = convert_double(num.c_str());
        if (p_invar_sites < 0 || p_invar_sites >= 1) {
            outError("p_invar must be in [0,1)");
        }
        return p_invar_sites;
    } else if (model_name.length() > posI+2 &&
               model_name[posI+2] != '+' &&
               model_name[posI+2] != '*') {
        outError("Wrong model name ", model_name);
    }
    return 0;
}

bool ModelInfoFromName::hasAscertainmentBiasCorrection() const {
    return model_name.find("+ASC") != std::string::npos;
}

bool ModelInfoFromName::hasRateHeterotachy() const {
    return findSubStr(model_name, "+H", "*H") != std::string::npos;
}

bool ModelInfoFromName::isFreeRate() const {
    return findSubStr(model_name, "+R", "*R") != std::string::npos;
}

bool ModelInfoFromName::isFrequencyMixture() const {
    return model_name.find("+FMIX") != string::npos;
}

bool ModelInfoFromName::isGammaModel() const {
    std::string::size_type posG  = model_name.find("+G");
    std::string::size_type posG2 = model_name.find("*G");
    if (posG != std::string::npos &&
        posG2 != std::string::npos) {
        stringstream s;
        auto posFirst = (posG < posG2) ? posG : posG2;
        s << "NOTE: both +G and *G were specified, continue with "
          << model_name.substr(posFirst,2);
        outWarning(s.str());
        if (posG2 < posG) {
            //Todo: port: posG = posG2;
            //fused_mix_rate = true;
        }
    }
    return posG  != std::string::npos ||
           posG2 != std::string::npos;
}

bool ModelInfoFromName::isInvariantModel() const {
    return model_name.find("+I") != std::string::npos;
}

bool ModelInfoFromName::isMixtureModel() const {
    return startsWith(model_name, "MIX");
}

bool ModelInfoFromName::isModelFinder() const {
    return model_name.empty() ||
           startsWith(model_name, "TEST") ||
           startsWith(model_name, "MF");
}

bool ModelInfoFromName::isModelFinderOnly() const {
    return contains(model_name,"ONLY")
    || (startsWith(model_name,"MF") &&
        !startsWith(model_name,"MFP"));
}

bool ModelInfoFromName::isPolymorphismAware() const {
    return posPOMO(model_name) != std::string::npos;
}

bool ModelInfoFromName::isWeissAndVonHaeselerTest() const {
    return model_name == "WHTEST";
}

ASCType ModelInfoFromName::extractASCType
    (std::string& leftover_name) const {
    auto posasc = model_name.find("+ASC_INF");
    if (posasc != std::string::npos) {
        leftover_name = model_name.substr(0, posasc)
                      + model_name.substr(posasc+8);
        return ASC_INFORMATIVE;
    }
    posasc = model_name.find("+ASC_MIS");
    if (posasc != std::string::npos) {
        leftover_name = model_name.substr(0, posasc)
                      + model_name.substr(posasc+8);
        return ASC_VARIANT_MISSING;
    }
    posasc = model_name.find("+ASC");
    ASSERT( posasc != std::string::npos);
    leftover_name = model_name.substr(0, posasc)
                  + model_name.substr(posasc+4);
    return ASC_VARIANT;
}

std::string ModelInfoFromName::extractMixtureModelList
    (std::string& leftover_name) const {
    ASSERT(startsWith(model_name, "MIX"));
    if (model_name[3] != OPEN_BRACKET) {
        outError("Mixture model name must start with 'MIX{'");
    }
    if (model_name.rfind(CLOSE_BRACKET) != model_name.length()-1) {
        outError("Close bracket not found at the end of ", model_name);
    }
    leftover_name = "MIX";
    //length is at least 5, since string starts MIX{ and ends },
    //so the next line is safe:
    return model_name.substr(4, model_name.length()-5);
}

std::string ModelInfoFromName::extractPolymorphicHeterozygosity
            (std::string& leftover_name) const {
    auto p_pos = findSubStr(model_name, "+P", "*P");
    ASSERT( p_pos != std::string::npos );
    leftover_name = model_name;
    std::string pomo_heterozygosity;
    if (model_name[p_pos+2] == '{') {
        std::string::size_type close_bracket = model_name.find("}", p_pos);
        if (close_bracket == std::string::npos) {
            std::cout << "Model string: " << model_name << std::endl;
            outError("No closing bracket in PoMo parameters.");
        }
        else {
            auto het_start = p_pos+3;
            auto het_len   = close_bracket-p_pos-3;
            pomo_heterozygosity = model_name.substr(het_start,het_len);
            leftover_name = model_name.substr(0, p_pos)
                          + model_name.substr(close_bracket+1);
        }
    }
    else {
        leftover_name = model_name.substr(0, p_pos)
                      + model_name.substr(p_pos + 2);
    }
    return pomo_heterozygosity;
}

void ModelInfoFromName::updateName(const std::string& name) {
    model_name = name;
}

YAMLFileParameter::YAMLFileParameter() : is_subscripted(false), value(0.0) {
}

std::string YAMLFileParameter::getSubscriptedVariableName(int subscript) const {
    std::stringstream subscripted_name;
    subscripted_name << name << "(" << subscript << ")";
    return subscripted_name.str();
}

bool YAMLFileParameter::isMatchFor(const std::string& match_name,
                                   ModelParameterType match_type) const {
    return (type == match_type &&
            string_to_lower(name) == match_name);
}

ModelVariable::ModelVariable(): value(0), is_fixed(false) {
}

ModelVariable::ModelVariable(ModelParameterType t,
                             const ModelParameterRange& r,
                             double v)
    : range(r), type(t), value(v), is_fixed(false) {
}

void ModelVariable::setValue(double v) {
    value = v;
}

void ModelVariable::markAsFixed() {
    is_fixed = true;
}

double ModelVariable::getValue() const {
    return value;
}

bool ModelVariable::isFixed() const {
    return is_fixed;
}

void StringMatrix::makeRectangular(size_t column_count) {
    size_t row_count = size();
    for (int row_num=0; row_num<row_count; ++row_num) {
        StrVector& row = at(row_num);
        if (row.size() != column_count) {
            row.resize(row_count, "");
        }
    }
}

void StringMatrix::makeSquare(bool reflect) {
    size_t row_count = size();
    size_t col_count = row_count;
    for (StrVector& row: *this) {
        if (col_count < row.size()) {
            col_count = row.size();
        }
    }
    for (size_t row_num = 0;
         row_num<size(); ++row_num ) {
        StrVector& row    = at(row_num);
        int old_col_count = row.size();
        if ( old_col_count < col_count ) {
            row.resize(col_count, "");
            if (reflect) {
                for (int col_num=old_col_count;
                     col_num<col_count; ++col_num) {
                    if ( col_num == row_num ) {
                        continue;
                    }
                    if (row_num < at(col_num).size()) {
                        row[col_num] = at(col_num)[row_num];
                    }
                }
            }
        }
    }
}

ModelInfoFromYAMLFile::ModelInfoFromYAMLFile()
    : sequence_type(SEQ_UNKNOWN), num_states(0), rate_matrix_rank(0)
    , frequency_type(FREQ_UNKNOWN), mixed_models(nullptr) {
}

ModelInfoFromYAMLFile::ModelInfoFromYAMLFile(const ModelInfoFromYAMLFile& rhs)
    : model_name(rhs.model_name), model_file_path(rhs.model_file_path)
    , citation(rhs.citation), DOI(rhs.DOI), url(rhs.url)
    , data_type_name(rhs.data_type_name), sequence_type(rhs.sequence_type)
    , num_states(rhs.num_states), reversible(rhs.reversible)
    , rate_matrix_rank(rhs.rate_matrix_rank)
    , rate_matrix_expressions(rhs.rate_matrix_expressions)
    , rate_matrix_formula(rhs.rate_matrix_formula)
    , tip_likelihood_rank(rhs.tip_likelihood_rank)
    , tip_likelihood_expressions(rhs.tip_likelihood_expressions)
    , tip_likelihood_formula(rhs.tip_likelihood_formula)
    , parameters(rhs.parameters), frequency_type(rhs.frequency_type)
    , variables(rhs.variables), mixed_models(nullptr) {
    if (rhs.mixed_models!=nullptr) {
        mixed_models = new MapOfModels(*mixed_models);
    }
}

ModelInfoFromYAMLFile::ModelInfoFromYAMLFile(const std::string& path)
    : model_file_path(path), sequence_type(SEQ_UNKNOWN), num_states(0)
    , rate_matrix_rank(0), frequency_type(FREQ_UNKNOWN)
    , mixed_models(nullptr) {
}

ModelInfoFromYAMLFile::~ModelInfoFromYAMLFile() {
    delete mixed_models;
}

bool ModelInfoFromYAMLFile::isMixtureModel() const {
    return mixed_models!=nullptr;
}

bool ModelInfoFromYAMLFile::isModelFinder() const {
    return false;
}

bool ModelInfoFromYAMLFile::isModelFinderOnly() const {
    return false;
}


bool ModelInfoFromYAMLFile::isReversible() const {
    return reversible;
}

void ModelInfoFromYAMLFile::updateName(const std::string& name) {
    model_name = name;
}

std::string ModelInfoFromYAMLFile::getLongName() const {
    return model_name + " from YAML model file " +
        model_file_path;
}

bool ModelInfoFromYAMLFile::hasDot(const char* name) const {
    return strstr(name, ".") != nullptr;
}

void ModelInfoFromYAMLFile::breakAtDot(const char* name,
                                       std::string& sub_model_name,
                                       const char* &remainder) const {
    auto dotpos = strstr(name, ".");
    if (dotpos==nullptr) {
        sub_model_name.clear();
        remainder = name;
    }
    else {
        sub_model_name = std::string(name, dotpos-name);
        remainder      = name + 1;
    }
}

ModelInfoFromYAMLFile::MapOfModels::const_iterator
ModelInfoFromYAMLFile::findMixedModel(const std::string& name) const {
    auto it = mixed_models->find(name);
    if (it==mixed_models->end()) {
        std::stringstream complaint;
        complaint << "Could not evaluate variable " << name
                  << " for model " << getLongName();
        throw ModelExpression::ModelException(complaint.str());
    }
    return it;
}

ModelInfoFromYAMLFile::MapOfModels::iterator
ModelInfoFromYAMLFile::findMixedModel(const std::string& name) {
    auto it = mixed_models->find(name);
    if (it==mixed_models->end()) {
        std::stringstream complaint;
        complaint << "Could not evaluate variable " << name
                  << " for model " << getLongName();
        throw ModelExpression::ModelException(complaint.str());
    }
    return it;
}

void ModelInfoFromYAMLFile::setNumberOfStatesAndSequenceType( int requested_num_states ) {
    if ( requested_num_states != 0 ) {
        num_states = requested_num_states;
    }
    if ( num_states == 0 ) {
        num_states = 4;
    }
    if (!data_type_name.empty()) {
        auto seq_type_requested = getSeqType(data_type_name.c_str());
        if (seq_type_requested!=SEQ_UNKNOWN) {
            sequence_type = seq_type_requested;
            num_states    = getNumStatesForSeqType(sequence_type, num_states);
        }
    }
    if (sequence_type==SEQ_UNKNOWN) {
        switch (num_states) {
            case 2:   sequence_type = SEQ_BINARY;  break;
            case 4:   sequence_type = SEQ_DNA;     break;
            case 20:  sequence_type = SEQ_PROTEIN; break;
            case 61:  sequence_type = SEQ_CODON;   break;
            default:  /* still, no idea */
                outWarning("Could not determine sequence type"
                           " for model " + model_name);
                break;
        }
    }
    forceAssign("num_states", num_states);
    forceAssign("numStates",  num_states);
}


const YAMLFileParameter* ModelInfoFromYAMLFile::findParameter
    ( const char* name, ModelParameterType type) const {
    size_t      param_count = parameters.size();
    std::string lower_name  = string_to_lower(name);
    for (size_t i = 0 ; i < param_count; ++i) {
        if (parameters[i].isMatchFor(lower_name, type)) {
            return &parameters[i];
        }
    }
    return nullptr;
}

void  ModelInfoFromYAMLFile::moveParameterToBack
    ( const char* name, ModelParameterType type) {
    size_t      param_count = parameters.size();
    std::string lower_name  = string_to_lower(name);
    size_t      i;
    for (i = 0 ; i < param_count; ++i) {
        if (parameters[i].isMatchFor(lower_name, type)) {
            break;
        }
    }
    if (i==param_count) {
        return; //Didn't find the parameter to be moved
    }
    YAMLFileParameter lift = parameters[i];
    for (size_t j = i+1; j<param_count; ++j) {
        if (parameters[j].type == type ) {
            parameters[i] = parameters[j];
            i = j;
        }
    }
    parameters[i] = lift;
}


bool ModelInfoFromYAMLFile::hasVariable(const char* name) const {
    if (hasDot(name) && mixed_models!=nullptr) {
        std::string sub_model_name;
        const char* var_name = nullptr;
        breakAtDot(name, sub_model_name, var_name);
        auto it = findMixedModel(sub_model_name);
        return it->second.hasVariable(var_name);
    }
    return variables.find(name) != variables.end();
}

bool ModelInfoFromYAMLFile::hasVariable(const std::string& name) const {
    if (hasDot(name.c_str()) && mixed_models!=nullptr) {
        std::string sub_model_name;
        const char* var_name = nullptr;
        breakAtDot(name.c_str(), sub_model_name, var_name);
        auto it = findMixedModel(sub_model_name);
        return it->second.hasVariable(var_name);
    }
    return variables.find(name) != variables.end();
}

double ModelInfoFromYAMLFile::getVariableValue(const std::string& name) const {
    return getVariableValue(name.c_str());
}

double ModelInfoFromYAMLFile::getVariableValue(const char* name) const {
    auto found = variables.find(name);
    if (found == variables.end()) {
        if (hasDot(name) && mixed_models!=nullptr) {
            std::string sub_model_name;
            const char* var_name = nullptr;
            breakAtDot(name, sub_model_name, var_name);
            auto it = findMixedModel(sub_model_name);
            return it->second.getVariableValue(var_name);
        }
        return 0.0;
    }
    return found->second.getValue();
}

bool ModelInfoFromYAMLFile::isVariableFixed(const std::string& name) const {
    auto found = variables.find(name);
    if (found == variables.end()) {
        if (hasDot(name.c_str()) && mixed_models!=nullptr) {
            std::string sub_model_name;
            const char* var_name = nullptr;
            breakAtDot(name.c_str(), sub_model_name, var_name);
            auto it = findMixedModel(sub_model_name);
            return it->second.isVariableFixed(var_name);
        }
        return false;
    }
    return found->second.isFixed();
}

void ModelInfoFromYAMLFile::addParameter(const YAMLFileParameter& p) {
    bool replaced = false;
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        if (it->name == p.name) {
            *it = p;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        parameters.emplace_back(p);
    }
    if (p.is_subscripted) {
        for (int i=p.minimum_subscript; i<=p.maximum_subscript; ++i) {
            std::string var_name = p.getSubscriptedVariableName(i);
            variables[var_name] = ModelVariable(p.type, p.range, p.value);
        }
    } else {
        variables[p.name] = ModelVariable(p.type, p.range, p.value);
    }
}

bool ModelInfoFromYAMLFile::isFrequencyParameter(const std::string& param_name) const {
    for ( auto p : parameters ) {
        if (string_to_lower(p.name) == string_to_lower(param_name)) {
            return p.type == ModelParameterType::FREQUENCY;
        }
    }
    return false;
}

void ModelInfoFromYAMLFile::setBounds(int param_count, double *lower_bound,
                                      double *upper_bound, bool *bound_check) const {
    int i = 1; //Rate parameter numbering starts at 1, see ModelMarkov
    for ( auto p : parameters ) {
        if (p.type == ModelParameterType::RATE) {
            for (int sub = p.minimum_subscript;
                 sub <= p.maximum_subscript; ++sub) {
                ASSERT( i<= param_count );
                lower_bound[i] = p.range.first;
                upper_bound[i] = p.range.second;
                bound_check[i] = false;
                ++i;
            }
        }
    }
}

void ModelInfoFromYAMLFile::updateVariables(const double* updated_values,
                                            int first_freq_index,
                                            int param_count) {
    int i = 1; //Rate parameter numbering starts at 1, see ModelMarkov
    ModelParameterType supported_types[] = {
        ModelParameterType::RATE, ModelParameterType::FREQUENCY };
        //FREQUENCY must be after RATE.
        //Todo: Where do weight parameters go?    Esepecially in mixture
        //      models
    for ( auto param_type : supported_types ) {
        if (param_type == FREQUENCY) {
            i = first_freq_index;
        }
        for ( auto p : parameters ) {
            if (p.type == param_type) {
                for (int sub = p.minimum_subscript;
                     sub <= p.maximum_subscript; ++sub) {
                    if ( i<= param_count ) {
                        std::string var_name  = p.getSubscriptedVariableName(sub);
                        double      var_value = updated_values[i];
                        if (!this->variables[var_name].isFixed()) {
                            this->variables[var_name].setValue(var_value);
                        }
                        ++i;
                    }
                    //For the last frequency parameter, if there is one,
                    //i will be equal to ( param_count + 1 ).
                }
            }
        }
    }
}

void ModelInfoFromYAMLFile::logVariablesTo(PhyloTree& report_to_tree) const {
    if (verbose_mode < VB_MIN) {
        return;
    }
    std::stringstream var_list;
    const char* sep = "Variables: ";
    for (auto itv : variables) {
        var_list << sep << itv.first << "=" << itv.second.getValue();
        sep = ", ";
    }
    std::string list = var_list.str();
    if (list.find("nan") != std::string::npos) {
        list += " ...?";
    }
    TREE_LOG_LINE(report_to_tree, YAMLModelVerbosity, list);
}

ModelVariable& ModelInfoFromYAMLFile::assign(const std::string& var_name,
                                             double value_to_set) {
    auto it = variables.find(var_name);
    if ( it == variables.end()) {
        if (hasDot(var_name.c_str()) && mixed_models!=nullptr) {
            std::string sub_model_name;
            const char* sub_model_var_name = nullptr;
            breakAtDot(var_name.c_str(), sub_model_name,
                       sub_model_var_name);
            auto it = findMixedModel(sub_model_name);
            return it->second.assign(sub_model_var_name, value_to_set);
        }

        std::stringstream complaint;
        complaint << "Could not assign"
                  << " to unrecognized variable " << var_name
                  << " of model " << model_name << ".";
        outError(complaint.str());
    }
    it->second.setValue(value_to_set);
    return it->second;
}

ModelVariable& ModelInfoFromYAMLFile::forceAssign (const std::string& var_name,
                                                   double value_to_set) {
    auto it = variables.find(var_name);
    if ( it == variables.end()) {
        if (hasDot(var_name.c_str()) && mixed_models!=nullptr) {
            std::string sub_model_name;
            const char* sub_model_var_name = nullptr;
            breakAtDot(var_name.c_str(), sub_model_name,
                       sub_model_var_name);
            auto it = findMixedModel(sub_model_name);
            return it->second.forceAssign(sub_model_var_name, value_to_set);
        }
        ModelVariable var(ModelParameterType::OTHER,
                          ModelParameterRange(), value_to_set);
        variables[var_name] = var;
        it = variables.find(var_name);
    }
    it->second.setValue(value_to_set);
    return it->second;
}

const StrVector& ModelInfoFromYAMLFile::getVariableNamesByPosition() const {
    variable_names.clear();
    ModelParameterType supported_types[] = {
        ModelParameterType::RATE, ModelParameterType::FREQUENCY };
        //FREQUENCY must be after RATE.
        //Todo: Where do weight parameters go?    Esepecially in mixture
        //      models
    for ( auto param_type : supported_types ) {
        for ( auto p : parameters ) {
            if (p.type == param_type) {
                for (int sub = p.minimum_subscript;
                     sub <= p.maximum_subscript; ++sub) {
                    std::string var_name  = p.getSubscriptedVariableName(sub);
                }
            }
        }
    }
    return variable_names;
}

ModelVariable& ModelInfoFromYAMLFile::assignByPosition(size_t position,
                                                       double value_to_set) {
    if (variable_names.size() < variables.size()) {
        getVariableNamesByPosition();
    }
    if (variable_names.size() <= position) {
        std::stringstream complaint;
        complaint << "Could not assign parameter " << (position+1)
                  << " as there are only " << variable_names.size()
                  << " parameters in model " << model_name << ".";
        outError(complaint.str());
    }
    return assign(variable_names[position], value_to_set);
}

bool ModelInfoFromYAMLFile::assignLastFrequency(double value) {
    //Ee-uw.  It's sad that this is needed!
    for ( auto pit = parameters.rbegin();
         pit!= parameters.rend(); ++pit ) {
        if (pit->type == ModelParameterType::FREQUENCY) {
            for (int sub = pit->maximum_subscript;
                 pit->minimum_subscript <= sub; --sub) {
                std::string var_name  = pit->getSubscriptedVariableName(sub);
                if (!this->variables[var_name].isFixed()) {
                    this->variables[var_name].setValue(value);
                    return true;
                }
                //For the last frequency parameter, if there is one,
                //i will be equal to ( param_count + 1 ).
            }
        }
    }
    return false;
}

std::string ModelInfoFromYAMLFile::getStringProperty(const char* name,
                                                     const char* default_value) const {
    auto it = string_properties.find(name);
    return it==string_properties.end() ? default_value : it->second;
}

int ModelInfoFromYAMLFile::getNumStates() const {
    //Todo: decide from the data type (or at least from data_type_name!)
    return 4; //but, for now, hardcoded!
}

int  ModelInfoFromYAMLFile::getTipLikelihoodMatrixRank() const {
    return tip_likelihood_rank;
}

void ModelInfoFromYAMLFile::computeTipLikelihoodsForState(int state, int num_states,
                                                          double* likelihoods) {
    std::stringstream complaint;
    
    int tip_states = getTipLikelihoodMatrixRank();
    if (state<0) {
        complaint << "Cannot calculate tip likelihoods for state " << state << ".";
    } else if (num_states<=state && tip_likelihood_formula.empty()) {
        complaint << "Cannot calculate tip likelihoods for state " << state
                  << " as there are only " << num_states << " states.";
    } else if (tip_states<=state && tip_likelihood_formula.empty() ) {
        complaint << "Cannot calculate tip likelihoods for state " << state
                  << " as tip likelihoods were provided"
                  << " only for " << tip_states << " states.";
    }
    if (!complaint.str().empty()) {
        outError(complaint.str());
    }
    StrVector dummyRow;
    StrVector& expr_row = tip_likelihood_expressions.empty()
                        ? dummyRow : tip_likelihood_expressions[state];
    
    typedef ModelExpression::InterpretedExpression Interpreter;

    forceAssign("row",        (double)state);
    ModelVariable& column_var = forceAssign("column", (double)0);
    for (int column=0; column<num_states; ++column) {
        column_var.setValue((double)column);
        std::string expr_string;
        if ( column < expr_row.size() ) {
            expr_string = expr_row[column];
        }
        if (expr_string.empty() && !tip_likelihood_formula.empty()) {
            expr_string = tip_likelihood_formula;
        }
        if (expr_string.empty()) {
            likelihoods[column] = (column==state) ? 1.0 : 0.0;
        } else {
            try {
                Interpreter interpreter(*this, expr_string);
                likelihoods[column] = interpreter.evaluate();
            }
            catch (ModelExpression::ModelException& x) {
                std::stringstream msg;
                msg << "Error parsing expression"
                    << " for tip likelihood matrix entry"
                    << " for (0-based) row "    << state << ","
                    << " and (0-based) column " << column << ":\n"
                    << x.getMessage();
                outError(msg.str());
            }
        }
    }
}

int ModelInfoFromYAMLFile::getRateMatrixRank() const {
    return rate_matrix_rank;
}

std::string ModelInfoFromYAMLFile::getParameterList(ModelParameterType param_type) const {
    std::stringstream list;
    appendParameterList(param_type, list);
    return list.str();
}

void ModelInfoFromYAMLFile::appendParameterList(ModelParameterType param_type,
                                                std::stringstream& list) const {
    const char* separator = "";
    for ( auto p : parameters ) {
        if (p.type == param_type) {
            if (p.is_subscripted) {
                for (int sub = p.minimum_subscript;
                     sub <= p.maximum_subscript; ++sub) {
                    std::string var_name = p.getSubscriptedVariableName(sub);
                    auto it = variables.find(var_name);
                    if (it!=variables.end()) {
                        const ModelVariable &v = it->second;
                        list << separator << var_name << "=" << v.getValue();
                        if (v.isFixed()) {
                            list << "(*)";
                        }
                        separator = ", ";
                    } else {
                        std::stringstream complaint;
                        complaint << "Variable " << var_name << " not found ";
                        outError(complaint.str());
                    }
                }
            } else {
                const std::string& var_name = p.name;
                auto it = variables.find(var_name);
                if (it!=variables.end()) {
                    const ModelVariable &v = it->second;
                    list << separator << p.name << "=" << v.getValue();
                    if (v.isFixed()) {
                        list << "(*)";
                    }
                    separator = ", ";
                } else {
                    std::stringstream complaint;
                    complaint << "Variable " << var_name << " not found ";
                    outError(complaint.str());
                }
            }
        }
    }
    
    //
    //Todo: Decide, is this right?  e.g. Rate parameters
    //      for a JC+GTR model might read "GTR={r(1)=4, r(2)=3, ...}"
    //
    if (this->mixed_models!=nullptr) {
        for (auto it=mixed_models->begin(); it!=mixed_models->end(); ++it) {
            list << separator << it->first;
            list << "={";
            it->second.appendParameterList(param_type, list);
            list << "}";
            separator = ", ";
        }
    }
}

const std::string& ModelInfoFromYAMLFile::getRateMatrixExpression
    ( int row, int col ) const {
    if (rate_matrix_expressions.empty()) {
        return rate_matrix_formula;
    }
    ASSERT( 0   <= row );
    ASSERT( row <  rate_matrix_expressions.size() );
    ASSERT( 0   <= col );
    ASSERT( col <  rate_matrix_expressions.size() );
    const StrVector& matrix_row = rate_matrix_expressions[row];
    if ( col < matrix_row.size() ) {
        return matrix_row[col];
    }
    //
    //possibly a triangular matrix?!
    //Todo: Don't the stationary frequencies have
    //to be taken into account, here?
    //
    std::swap(row, col);
    const StrVector& other_matrix_row = rate_matrix_expressions[row];
    ASSERT ( col <  other_matrix_row.size() );
    return other_matrix_row[col];
}

const std::string& ModelInfoFromYAMLFile::getName() const {
    return model_name;
}

void ModelListFromYAMLFile::loadFromFile (const char* file_path,
                                          PhyloTree* report_to_tree) {
    YAML::Node yaml_model_list = YAML::LoadFile(file_path);
    ModelFileLoader loader(file_path);
    try {
        if (!yaml_model_list.IsSequence()) {
            throw YAML::Exception(yaml_model_list.Mark(),
                                  "list '[...]' expected");
        }
        for (auto node : yaml_model_list) {
            if (!(node["substitutionmodel"])) {
                continue;
            }
            std::string yaml_model_name = node["substitutionmodel"].Scalar();
            TREE_LOG_LINE(*report_to_tree, YAMLModelVerbosity,
                          "Parsing YAML model " << yaml_model_name);
            ModelInfoFromYAMLFile &y = models_found[yaml_model_name]
                                     = ModelInfoFromYAMLFile();
            loader.parseYAMLSubstitutionModel(node, yaml_model_name, y, *this,
                                              nullptr, report_to_tree);
        }
    }
    catch (YAML::Exception &e) {
        outError(e.what());
    }
    catch (ModelExpression::ModelException& x) {
        outError(x.getMessage());
    }
}

bool ModelListFromYAMLFile::isModelNameRecognized (const char* model_name) {
    size_t i = 0;
    while (model_name[i]!='\0' && model_name[i]!='{') {
        ++i;
    }
    bool recognized = models_found.find(std::string(model_name, i)) != models_found.end();
    return recognized;
}

template <class S> class YAMLModelWrapper: public S {
protected:
    ModelInfoFromYAMLFile model_info;
    PhyloTree*            report_tree;
public:
    typedef S super;
    using   S::freq_type;
    using   S::num_params;
    using   S::num_states;
    using   S::rates;
    using   S::state_freq;
    
    using   S::getNDim;
    using   S::getNumberOfRates;
    using   S::setRateMatrix;
    
    YAMLModelWrapper(const ModelInfoFromYAMLFile& info,
                     PhyloTree* report_to_tree)
        : super(report_to_tree, report_to_tree)
        , model_info(info), report_tree(report_to_tree) {
    }
    
    void acceptParameterList(std::string parameter_list,
                             PhyloTree* report_to_tree) {
        //parameter_list is passed by value so it can be modified
        trimString(parameter_list);
        if (startsWith(parameter_list, "{") &&
            endsWith(parameter_list, "}")) {
            parameter_list = parameter_list.substr(1, parameter_list.length()-2);
        }
        size_t param_list_length = parameter_list.length();
        size_t i                 = 0;
        int    bracket_depth     = 0;
        typedef ModelExpression::InterpretedExpression Expr;
        std::vector<Expr> expr_list;
        while (i<param_list_length) {
            size_t j = i;
            for (;j<param_list_length &&
                 (parameter_list[j]!=',' || 0<bracket_depth); ++j) {
                char ch = parameter_list[j];
                if (ch=='(') {
                    ++bracket_depth;
                }
                else if (ch==')') {
                    --bracket_depth;
                }
            }
            std::string param = parameter_list.substr(i, j-i);
            expr_list.emplace_back(model_info, param);
            i = j + 1;
        }
        bool fix = !report_to_tree->params->optimize_from_given_params;
        size_t position = 0;
        for (Expr& ix : expr_list) {
            ModelExpression::Expression* x = ix.expression();
            if (x->isAssignment()) {
                typedef ModelExpression::Assignment A;
                typedef ModelExpression::Variable V;
                A*             a       = dynamic_cast<A*>(x);
                V*             xv      = a->getTargetVariable();
                double         setting = a->getExpression()->evaluate();
                ModelVariable& mv      = model_info.assign(xv->getName(), setting);
                if (fix) {
                    mv.markAsFixed();
                }
                TREE_LOG_LINE(*report_to_tree, YAMLModelVerbosity,
                              "Set " << xv->getName() << " to " << setting << " by name." );
            } else {
                double         setting  = x->evaluate();
                ModelVariable& mv       = model_info.assignByPosition(position, setting);
                string         var_name = model_info.getVariableNamesByPosition()[position];
                //No need for an index check for var_name's look-up, because
                //assignByPosition would already have bombed if it were out of bounds.
                if (fix) {
                    mv.markAsFixed();
                }
                TREE_LOG_LINE(*report_to_tree, YAMLModelVerbosity,
                              "Set " << var_name << " to " << setting << " by position." );
                ++position;
            }
        }
        if (!expr_list.empty()) {
            setRateMatrixFromModel();
        }
    }
    
    virtual void setBounds(double *lower_bound, double *upper_bound,
                            bool *bound_check) {
        //
        int ndim = getNDim();
        for (int i = 1; i <= ndim; ++i) {
            lower_bound[i] = MIN_RATE;
            upper_bound[i] = MAX_RATE;
            bound_check[i] = false;
        }
        model_info.setBounds(ndim, lower_bound,
                             upper_bound, bound_check);
    }
    
    virtual bool getVariables(double *variables) {
        bool changed = false;
        if (num_params > 0) {
            int num_all = getNumberOfRates();
            for (int i = 0; i < num_all; i++) {
                if (rates[i] != variables[i] ) {
                    TREE_LOG_LINE(*report_tree, VB_MAX,
                                  "  estimated rates[" << i << "] changing from "
                                  << rates[i] << " to " << variables[i]);
                    rates[i] = variables[i];
                    changed  = true;
                }
            }
        }
        int ndim = getNDim();
        int first_freq_index = (ndim-num_states+2);
        if (freq_type == FREQ_ESTIMATE) {
            auto read_freq = variables+first_freq_index;
            for (int i=0; i<num_states-1; ++i) {
                if (state_freq[i]!=read_freq[i]) {
                    TREE_LOG_LINE(*report_tree, VB_MAX,
                                  "  estimated freqs[" << i << "]"
                                  << " changing from " << state_freq[i]
                                  << " to " << read_freq[i]);
                    state_freq[i] = read_freq[i];
                    changed       = true;
                }
            }
            //Set the last frequency to the residual
            //(one minus the sum of the others)
            if (scaleStateFreq()) {
                changed = true;
                model_info.assignLastFrequency(state_freq[num_states-1]);
            }
        } else {
            changed |= freqsFromParams(state_freq, variables+num_params+1,
                                       freq_type);
        }
        TREE_LOG_LINE(*report_tree, VB_MAX, "");
        if (changed) {
            model_info.updateVariables(variables, first_freq_index, getNDim());
            model_info.logVariablesTo(*report_tree);
            setRateMatrixFromModel();
        }
        return changed;
    }
    
    virtual bool scaleStateFreq() {
        // make the frequencies sum to 1
        bool changed = false;
        double sum = 0.0;
        for (int i = 0; i < num_states-1; ++i) {
            sum += state_freq[i];
        }
        if (1.0<sum) {
            sum += state_freq[num_states-1];
            changed = true;
            for (int i = 0; i < num_states; ++i) {
                state_freq[i] /= sum;
            }
        } else {
            //Set last state frequency to 1.0 minus
            //the sum of the others
            double residual = 1.0 - sum;
            if (state_freq[num_states-1] != residual) {
                state_freq[num_states-1] = residual;
                changed = true;
            }
        }
        return changed;
    }

    virtual void setVariables(double *variables) {
        if (num_params > 0) {
            for (int i=0; i<num_params; ++i) {
                variables[i] = rates[i];
            }
        }
        if (freq_type == FREQ_ESTIMATE) {
            // 2015-09-07: relax the sum of state_freq to be 1,
            // this will be done at the end of optimization
            int ndim = getNDim();
            memcpy(variables+(ndim-num_states+2), state_freq,
                   (num_states-1)*sizeof(double));
        } else {
            paramsFromFreqs(variables+num_params+1,
                            state_freq, freq_type);
        }
    }
    
    void setRateMatrixFromModel() {
        auto rank = model_info.getRateMatrixRank();
        ASSERT( rank == num_states);
        
        DoubleVector      rates;
        const char*       separator = "";
        std::stringstream trace;
        trace << "Rate Matrix: { ";
        
        model_info.forceAssign("num_states", (double)num_states);
        ModelVariable& row_var    = model_info.forceAssign("row",    (double)0);
        ModelVariable& column_var = model_info.forceAssign("column", (double)0);
        
        for (int row = 0; row < rank; ++row) {
            row_var.setValue((double)row);
            for (int col = 0; col < rank; ++col) {
                column_var.setValue((double)col);
                if (col != row) {
                    std::string expr_string =
                        model_info.getRateMatrixExpression(row,col);
                    typedef ModelExpression::InterpretedExpression Interpreter;
                    try {
                        Interpreter interpreter(model_info, expr_string);
                        double entry = interpreter.evaluate();
                        rates.push_back(entry);
                        trace << separator << entry;
                    }
                    catch (ModelExpression::ModelException& x) {
                        std::stringstream msg;
                        msg << "Error parsing expression"
                            << " for " << model_info.getName()
                            << " rate matrix entry"
                            << " for row "    << (row + 1) << ","
                            << " and column " << (col + 1) << ": "
                            << x.getMessage();
                        outError(msg.str());
                    }
                } else {
                    trace << separator << "-";
                }
                separator = ", ";
            }
        }
        trace << " }";
        TREE_LOG_LINE(*report_tree, VB_MAX, trace.str());
        setRateMatrix(rates.data());
    }
    
    virtual void computeTipLikelihood(PML::StateType state, double *state_lk) {
        int state_num = static_cast<int>(state);
        if ( state_num < model_info.getTipLikelihoodMatrixRank()) {
            model_info.computeTipLikelihoodsForState(state, num_states, state_lk);
        } else if (state_num < num_states) {
            // single state
            memset(state_lk, 0, num_states*sizeof(double));
            state_lk[state] = 1.0;
        } else {
            // unknown state
            for (int i = 0; i < num_states; i++) {
                state_lk[i] = 1.0;
            }
        }
    }
    
    virtual void writeInfo(ostream &out) {
        auto rates = model_info.getParameterList(ModelParameterType::RATE);
        if (!rates.empty()) {
            out << "Rate parameters: " << rates << std::endl;
        }
        auto freqs = model_info.getParameterList(ModelParameterType::FREQUENCY);
        if (!freqs.empty()) {
            out << "State frequencies: " << freqs << std::endl;
        }
    }
};
    
class YAMLModelDNA: public YAMLModelWrapper<ModelDNA> {
public:
    typedef YAMLModelWrapper<ModelDNA> super;
    YAMLModelDNA(const char *model_name, string model_params,
                 StateFreqType freq, string freq_params,
                 PhyloTree *tree, PhyloTree* report_to_tree,
                 const ModelInfoFromYAMLFile& info): super(info, report_to_tree) {
        init(model_name, model_params, freq,
             freq_params, report_to_tree);
        setRateMatrixFromModel();
    }
};

class YAMLModelDNAError: public YAMLModelWrapper<ModelDNAError> {
public:
    typedef YAMLModelWrapper<ModelDNAError> super;
    YAMLModelDNAError(const char *model_name, string model_params,
                      StateFreqType freq, string freq_params,
                      PhyloTree *tree, PhyloTree* report_to_tree,
                      const ModelInfoFromYAMLFile& info): super(info, report_to_tree) {
        init(model_name, model_params, freq,
             freq_params, report_to_tree);
        
        setRateMatrixFromModel();
    }
    bool getVariables(double *variables) {
        bool changed = super::getVariables(variables);
        if (changed && !fix_epsilon) {
            epsilon = model_info.getVariableValue("epsilon");
        }
        return changed;
    }
};

class YAMLModelProtein: public YAMLModelWrapper<ModelProtein> {
public:
    typedef YAMLModelWrapper<ModelProtein> super;
    YAMLModelProtein(ModelsBlock* block,
                     const char *model_name, string model_params,
                     StateFreqType freq, string freq_params,
                     PhyloTree *tree, PhyloTree* report_to_tree,
                     const ModelInfoFromYAMLFile& info): super(info, report_to_tree) {
        setModelsBlock(block);
        setNumberOfStates(20);
        setReversible(info.isReversible());
        init(model_name, model_params, freq,
             freq_params, report_to_tree);
        setNumberOfStates(20); //Why again?
        setRateMatrixFromModel();
    }
};


bool ModelListFromYAMLFile::hasModel(const std::string& model_name) const {
    return models_found.find(model_name) != models_found.end();
}

const ModelInfoFromYAMLFile&
    ModelListFromYAMLFile::getModel(const std::string& model_name) const {
    auto it = models_found.find(model_name);
    ASSERT(it != models_found.end());
    return it->second;
}

namespace {
    void extractModelNameAndParameters(const char* model_plus_params,
                                       std::string& name,
                                       std::string& params) {
        size_t i = 0;
        while (model_plus_params[i]!='\0' && model_plus_params[i]!='{') {
            ++i;
        }
        name   = std::string(model_plus_params,i);
        params = std::string(model_plus_params+i);
        //includes the { at front and the } at back.
    }
};

ModelMarkov* ModelListFromYAMLFile::getModelByName(const char* model_name,   PhyloTree *tree,
                                                   const char* model_params, StateFreqType freq_type,
                                                   const char* freq_params,  ModelsBlock* models_block,
                                                   PhyloTree* report_to_tree) {
    std::string name;
    std::string parameter_list;
    extractModelNameAndParameters(model_name, name, parameter_list);
    ModelInfoFromYAMLFile& model_info = models_found[model_name];
    if (0<strlen(model_params) || 0<strlen(freq_params)) {
        TREE_LOG_LINE(*report_to_tree, YAMLModelVerbosity,
                      "Model Params: " << model_params
                      << " Freq Params: " << freq_params);
        if (parameter_list.empty()) {
            parameter_list = model_params;
        }
    }
    
    if (freq_type == FREQ_UNKNOWN) {
        freq_type = model_info.frequency_type;
    }
    
    switch (model_info.sequence_type) {
        case SeqType::SEQ_DNA:
            return getDNAModel(model_info, parameter_list,
                               freq_type, tree, report_to_tree);
        case SeqType::SEQ_PROTEIN:
            return getProteinModel(model_info, parameter_list,
                                   freq_type, tree, models_block,
                                   report_to_tree);
        default:
            outError("YAML model uses unsupported sequence type");
            return nullptr;
    };
}
    
ModelMarkov* ModelListFromYAMLFile::getDNAModel(ModelInfoFromYAMLFile& model_info,
                                                const std::string& parameter_list,
                                                StateFreqType freq_type,
                                                PhyloTree* tree,
                                                PhyloTree* report_to_tree) {
    ModelMarkov* model = nullptr;
    string dummy_rate_params;
    string dummy_freq_params;
    
    const YAMLFileParameter* p =
        model_info.findParameter("epsilon",
                                 ModelParameterType::RATE);
    if (p!=nullptr) {
        if (p->is_subscripted) {
            outError("epsilon parameter for DNA+error model"
                     "may not be subscripted");
        }
        double epsilon = p->value;
        bool epsilon_is_fixed = model_info.isVariableFixed("epsilon");
        model_info.moveParameterToBack("epsilon",
                                       ModelParameterType::RATE);
        YAMLModelDNAError* emodel;
        emodel = new YAMLModelDNAError("", dummy_rate_params, freq_type,
                                      dummy_freq_params, tree,
                                      report_to_tree, model_info);
        std::string error_model = model_info.getStringProperty("errormodel", "+E");
        emodel->setEpsilon(epsilon, epsilon_is_fixed, error_model);
        TREE_LOG_LINE(*report_to_tree, YAMLModelVerbosity,
                      "epsilon is " << epsilon
                      << ", fixed is " << epsilon_is_fixed
                      << ", and errormodel is " << error_model);
        emodel->acceptParameterList(parameter_list, report_to_tree);
        model = emodel;
    }
    else {
        YAMLModelDNA* dmodel;
        dmodel = new YAMLModelDNA("", dummy_rate_params, freq_type,
                                  dummy_freq_params, tree,
                                  report_to_tree, model_info);
        dmodel->acceptParameterList(parameter_list, report_to_tree);
        model = dmodel;
    }
    return model;
}

ModelMarkov* ModelListFromYAMLFile::getProteinModel(ModelInfoFromYAMLFile& model_info,
                                                    const std::string& parameter_list,
                                                    StateFreqType freq_type,
                                                    PhyloTree* tree,
                                                    ModelsBlock* models_block,
                                                    PhyloTree* report_to_tree) {
    ModelMarkov* model = nullptr;
    string dummy_rate_params;
    string dummy_freq_params;
    
    YAMLModelProtein* pmodel;
    pmodel = new YAMLModelProtein(models_block, "", dummy_rate_params, freq_type,
                                  dummy_freq_params, tree, report_to_tree,
                                  model_info);
    pmodel->acceptParameterList(parameter_list, report_to_tree);
    model = pmodel;
    
    return model;
}