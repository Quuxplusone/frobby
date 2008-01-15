#ifndef IRREDUCIBLE_DECOM_FACADE_GUARD
#define IRREDUCIBLE_DECOM_FACADE_GUARD

#include "Facade.h"
#include <vector>

class IrreducibleDecomParameters;
class BigIdeal;
class Strategy;
class TermTranslator;
class SliceStrategy;
class Ideal;
class TermConsumer;

class IrreducibleDecomFacade : private Facade {
 public:
  IrreducibleDecomFacade(bool printActions,
			 const IrreducibleDecomParameters& parameters);

  // These all clear ideal.
  void computeIrreducibleDecom(BigIdeal& ideal, FILE* out);
  void computeAlexanderDual(BigIdeal& ideal, FILE* out);
  void computeAlexanderDual(BigIdeal& ideal,
							const vector<mpz_class>& point, FILE* out);
  void computeFrobeniusNumber(const vector<mpz_class>& instance,
			      BigIdeal& ideal, 
			      mpz_class& frobeniusNumber);

  void computeIrreducibleDecom(Ideal& ideal, TermConsumer* consumer);

 private:
  void computeIrreducibleDecom(Ideal& ideal,
							   TermConsumer* consumer,
							   bool preMinimized );
  void computeAlexanderDual(BigIdeal& ideal,
							const vector<mpz_class>& point,
							bool useLcm,
							FILE* out);

  void runLabelAlgorithm(Ideal& ideal, Strategy* strategy);
  void runSliceAlgorithm(Ideal& ideal, SliceStrategy* strategy);

  const IrreducibleDecomParameters& _parameters;
};

#endif
