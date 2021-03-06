// fts_spec.h

/**
*    Copyright (C) 2012 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*    As a special exception, the copyright holders give permission to link the
*    code of portions of this program with the OpenSSL library under certain
*    conditions as described in each individual source file and distribute
*    linked combinations including the program with the OpenSSL library. You
*    must comply with the GNU Affero General Public License in all respects for
*    all of the code used other than as permitted herein. If you modify file(s)
*    with this exception, you may extend this exception to your version of the
*    file(s), but you are not obligated to do so. If you do not wish to do so,
*    delete this exception statement from your version. If you delete this
*    exception statement from all source files in the program, then also delete
*    it in the license file.
*/

#pragma once

#include <map>
#include <vector>
#include <string>

#include "mongo/db/fts/fts_language.h"
#include "mongo/db/fts/fts_util.h"
#include "mongo/db/fts/stemmer.h"
#include "mongo/db/fts/stop_words.h"
#include "mongo/db/fts/tokenizer.h"
#include "mongo/platform/unordered_map.h"

namespace mongo {

    namespace fts {

        extern const double MAX_WEIGHT;
        extern const double MAX_WORD_WEIGHT;

        typedef std::map<string,double> Weights; // TODO cool map

        typedef unordered_map<string,double> TermFrequencyMap;

        struct ScoreHelperStruct {
            ScoreHelperStruct()
                : freq(0), count(0), exp(0){
            }
            double freq;
            double count;
            double exp;
        };
        typedef unordered_map<string,ScoreHelperStruct> ScoreHelperMap;

        class FTSSpec {

            struct Tools {
                Tools( const FTSLanguage& _language,
                       const Stemmer* _stemmer,
                       const StopWords* _stopwords )
                    : language( _language )
                    , stemmer( _stemmer )
                    , stopwords( _stopwords ) {}

                const FTSLanguage& language;
                const Stemmer* stemmer;
                const StopWords* stopwords;
            };

        public:
            FTSSpec( const BSONObj& indexInfo );

            bool wildcard() const { return _wildcard; }
            const FTSLanguage& defaultLanguage() const { return *_defaultLanguage; }
            const string& languageOverrideField() const { return _languageOverrideField; }

            size_t numExtraBefore() const { return _extraBefore.size(); }
            const std::string& extraBefore( unsigned i ) const { return _extraBefore[i]; }

            size_t numExtraAfter() const { return _extraAfter.size(); }
            const std::string& extraAfter( unsigned i ) const { return _extraAfter[i]; }

            /**
             * Calculates term/score pairs for a BSONObj as applied to this spec.
             * - "obj": the BSONObj to traverse; can be a subdocument or array
             * - "parentLanguage": nearest enclosing document "language" spec for obj
             * - "parentPath": obj's dotted path in containing document
             * - "isArray": true if obj is an array
             * - "term_freqs": out-parameter to store results
             */
            void scoreDocument( const BSONObj& obj,
                                const FTSLanguage& parentLanguage,
                                const string& parentPath,
                                bool isArray,
                                TermFrequencyMap* term_freqs ) const;

            /**
             * given a query, pulls out the pieces (in order) that go in the index first
             */
            Status getIndexPrefix( const BSONObj& filter, BSONObj* out ) const;

            const Weights& weights() const { return _weights; }

            static BSONObj fixSpec( const BSONObj& spec );

        private:
            //
            // Helper methods.  Invoked for TEXT_INDEX_VERSION_2 spec objects only.
            //

            /**
             * Get the language override for the given BSON doc.  If no language override is
             * specified, returns currentLanguage.
             */
            const FTSLanguage& _getLanguageToUseV2( const BSONObj& userDoc,
                                                    const FTSLanguage& currentLanguage ) const;

            /**
             * Calculate the term scores for 'raw' and update 'term_freqs' with the result.  Parses
             * 'raw' using 'tools', and weights term scores based on 'weight'.
             */
            void _scoreStringV2( const Tools& tools,
                                 const StringData& raw,
                                 TermFrequencyMap* term_freqs,
                                 double weight ) const;

            //
            // Deprecated helper methods.  Invoked for TEXT_INDEX_VERSION_1 spec objects only.
            //

            void _scoreStringV1( const Tools& tools,
                                 const StringData& raw,
                                 TermFrequencyMap* docScores,
                                 double weight ) const;

            bool _weightV1( const StringData& field, double* out ) const;

            void _scoreRecurseV1( const Tools& tools,
                                  const BSONObj& obj,
                                  TermFrequencyMap* term_freqs ) const;

            void _scoreDocumentV1( const BSONObj& obj, TermFrequencyMap* term_freqs ) const;

            const FTSLanguage& _getLanguageToUseV1( const BSONObj& userDoc ) const;

            static BSONObj _fixSpecV1( const BSONObj& spec );

            //
            // Instance variables.
            //

            TextIndexVersion _textIndexVersion;

            const FTSLanguage* _defaultLanguage;
            string _languageOverrideField;
            bool _wildcard;

            // _weights stores a mapping between the fields and the value as a double
            // basically, how much should an occurence of (query term) in (field) be worth
            Weights _weights;

            // other fields to index
            std::vector<string> _extraBefore;
            std::vector<string> _extraAfter;
        };

    }
}
