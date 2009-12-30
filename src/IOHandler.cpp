/* Frobby: Software for monomial ideal computations.
   Copyright (C) 2007 Bjarke Hammersholt Roune (www.broune.com)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see http://www.gnu.org/licenses/.
*/
#include "stdinc.h"
#include "IOHandler.h"

// TODO: clean this up
#include "Scanner.h"
#include "BigIdeal.h"
#include "TermTranslator.h"
#include "Ideal.h"
#include "Term.h"
#include "TermConsumer.h"
#include "VarNames.h"
#include "CoefTermConsumer.h"
#include "Polynomial.h"
#include "BigPolynomial.h"
#include "NameFactory.h"
#include "error.h"
#include "FrobbyStringStream.h"
#include "ElementDeleter.h"
#include "BigTermConsumer.h"
#include "BigTermRecorder.h"
#include "TranslatingTermConsumer.h"
#include "IrreducibleIdealSplitter.h"
#include "DataType.h"
#include "IdealConsolidator.h"
#include "CoefBigTermConsumer.h"
#include "TranslatingCoefTermConsumer.h"
#include "CountingIOHandler.h"

#include "NewMonosIOHandler.h"
#include "MonosIOHandler.h"
#include "Macaulay2IOHandler.h"
#include "Fourti2IOHandler.h"
#include "NullIOHandler.h"
#include "CoCoA4IOHandler.h"
#include "SingularIOHandler.h"

IOHandler::~IOHandler() {
}

void IOHandler::readIdeal(Scanner& in, BigTermConsumer& consumer) {
  doReadIdeal(in, consumer);
}

void IOHandler::readIdeals(Scanner& in, BigTermConsumer& consumer) {
  doReadIdeals(in, consumer);
}

void IOHandler::readTerm
(Scanner& in, const VarNames& names, vector<mpz_class>& term) {
  doReadTerm(in, names, term);
}

void IOHandler::readPolynomial(Scanner& in, CoefBigTermConsumer& consumer) {
  doReadPolynomial(in, consumer);
}

void IOHandler::readSatBinomIdeal(Scanner& in, SatBinomConsumer& consumer) {
  doReadSatBinomIdeal(in, consumer);
}

void IOHandler::writeTerm(const vector<mpz_class>& term,
						  const VarNames& names, FILE* out) {
  doWriteTerm(term, names, out);
}

bool IOHandler::hasMoreInput(Scanner& in) const {
  return doHasMoreInput(in);
}

const char* IOHandler::getName() const {
  return doGetName();
}

const char* IOHandler::getDescription() const {
  return doGetDescription();
}

auto_ptr<BigTermConsumer> IOHandler::createIdealWriter(FILE* out) {
  if (!supportsOutput(DataType::getMonomialIdealType())) {
	FrobbyStringStream errorMsg;
	errorMsg << "The " << getName()
			 << " format does not support output of a monomial ideal.";
	reportError(errorMsg);
  }
  return doCreateIdealWriter(out);
}

auto_ptr<CoefBigTermConsumer> IOHandler::createPolynomialWriter(FILE* out) {
  if (!supportsOutput(DataType::getPolynomialType())) {
	FrobbyStringStream errorMsg;
	errorMsg << "The " << getName()
			 << " format does not support output of a polynomial.";
	reportError(errorMsg);
  }
  return doCreatePolynomialWriter(out);
}

bool IOHandler::supportsInput(const DataType& type) const {
  return doSupportsInput(type);
}

bool IOHandler::supportsOutput(const DataType& type) const {
  return doSupportsOutput(type);
}

namespace {
  typedef NameFactory<IOHandler> IOHandlerFactory;
  IOHandlerFactory getIOHandlerFactory() {
	IOHandlerFactory factory;

	nameFactoryRegister<Macaulay2IOHandler>(factory);
	nameFactoryRegister<CoCoA4IOHandler>(factory);
	nameFactoryRegister<SingularIOHandler>(factory);
	nameFactoryRegister<MonosIOHandler>(factory);
	nameFactoryRegister<NewMonosIOHandler>(factory);
	nameFactoryRegister<Fourti2IOHandler>(factory);
	nameFactoryRegister<NullIOHandler>(factory);
	nameFactoryRegister<CountingIOHandler>(factory);

	return factory;
  }
}

auto_ptr<IOHandler> createIOHandler(const string& name) {
  auto_ptr<IOHandler> handler = getIOHandlerFactory().create(name);
  if (handler.get() == 0)
	throwError<UnknownFormatException>("Unknown format \"" + name + "\".");	
  return handler;
}

void getIOHandlerNames(vector<string>& names) {
  getIOHandlerFactory().getNamesWithPrefix("", names);
}

void readFrobeniusInstance(Scanner& in, vector<mpz_class>& numbers) {
  numbers.clear();

  string number;
  mpz_class n;
  while (!in.matchEOF()) {
	in.readInteger(n);

    if (n <= 1) {
	  FrobbyStringStream errorMsg;
	  errorMsg << "Read the number " << n
			   << " while reading Frobenius instance. "
			   << "Only integers strictly larger than 1 are valid.";
	  reportSyntaxError(in, errorMsg);
    }

    numbers.push_back(n);
  }

  if (numbers.empty())
	reportSyntaxError
	  (in, "Read empty Frobenius instance, which is not allowed.");

  mpz_class gcd = numbers[0];
  for (size_t i = 1; i < numbers.size(); ++i)
    mpz_gcd(gcd.get_mpz_t(), gcd.get_mpz_t(), numbers[i].get_mpz_t());

  if (gcd != 1) {
	// Maybe not strictly speaking a syntax error, but that category
	// of errors still fits best.
	FrobbyStringStream errorMsg;
	errorMsg << "The numbers in the Frobenius instance are not "
			 << "relatively prime. They are all divisible by "
			 << gcd << '.';
	reportSyntaxError(in, errorMsg);
  }
}

string autoDetectFormat(Scanner& in) {
  // We guess based on the initial non-whitespace character. We detect
  // more than the correct initial character to try to guess the
  // intended format in the face of mistakes.
  in.eatWhite();
  switch (in.peek()) {
  case 'U': // correct
  case 'u': // incorrect
	return CoCoA4IOHandler::staticGetName();

  case 'r': // correct
	return SingularIOHandler::staticGetName();

  case '(': // correct
  case 'l': // incorrect
  case ')': // incorrect
	return NewMonosIOHandler::staticGetName();

  case '0': case '1': case '2': case '3': case '4': // correct
  case '5': case '6': case '7': case '8': case '9': // correct
  case '+': case '-': // incorrect
	return Fourti2IOHandler::staticGetName();

  case 'v': // correct
	return MonosIOHandler::staticGetName();

  case 'R': // correct
  default: // incorrect
	return Macaulay2IOHandler::staticGetName();
  }
}
