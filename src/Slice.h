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
#ifndef SLICE_GUARD
#define SLICE_GUARD

#include "Ideal.h"
#include "Term.h"

class Projection;

/** This class represents a slice, which is the central data structure
 of the %Slice Algorithm.

 To be precise, a slice is mathematically a 3-tuple \f$(I,S,q)\f$
 where \f$I\f$ and \f$S\f$ are monomial ideals and \f$q\f$ is a
 monomial. Each slice then represents some part of the output, which
 is known as its content. Usually the content is a set, and the
 content obeys the pivot split equation, which is \f[
   \con I S q = \con {I:p} {S:p} {qp} \cup \con I {S+p} q
 \f] where \f$p\f$ is a monomial called the pivot, and the union is
 disjoint. There are three slices in this equation, which are known in
 order from left to right as the current slice, the inner slice and
 the outer slice.

 The ideal \f$I\f$ is regarded as the base data of the slice, and is
 known as simply the ideal of the slice. The ideal \f$S\f$ is regarded
 as specifying monomials that are not to be regarded as part of the
 content, i.e. \f$S\f$ is subtracted from the content, so \f$S\f$ is
 known as the subtract of the slice. The monomial \f$q\f$ is
 multiplied onto the content, so it is known as the multiply of the
 slice. These parts of the slice do not actually have names, since in
 mathematics they are conveniently referred to as simply \f$I\f$,
 \f$S\f$ and \f$q\f$. We are introducing names for these symbols to
 have something to call them in the code.

 There are two base cases of the %Slice Algorithm. A trivial base case
 slice is when \f$\lcm(min(I))\f$ is not divisible by some variable,
 i.e. some variable does not appear in any element of
 \f$\min(I)\f$. In this case the content is empty.

 A non-trivial base case is when \f$I\f$ is square-free and the base case
 is not trivial. This is equivalent to \f$\lcm(min(I))\f$ being equal
 to the product of all variables in the ambient polynomial ring.

 The %Slice Algorithm has a notion of simplification, which is to
 replace a slice with a different slice that is simpler and has the
 same content.

 This class implements the notions of the %Slice Algorithm that are
 common among different versions of the %Slice Algorithm, while leaving
 derived classes to introduce code that is specific to a particular
 version. A derivative of Slice thus cooperates with a derivative of
 SliceStrategy to implement a specific version of the %Slice Algorithm.

 As the kind of output generated by a non-trivial base case slice
 depends on what is being computed, the general Slice interface does
 not provide a way to obtain the output. The suggested mechanism is
 for each slice derivative to store a Consumer and to provide the
 output to that consumer.
*/
class Slice {
 public:
  /** Construct the slice \f$(\ideal 0, \ideal 0, 1)\f$ in a ring of
   zero variables.
  */
  Slice();

  /** Construct the slice \f$(\codeVar{ideal}, \codeVar{subtract},
   \codeVar{multiply})\f$.
  */
  Slice(const Ideal& ideal, const Ideal& subtract, const Term& multiply);

  virtual ~Slice();

  /** @name Accessors */
  //@{

  /** Returns the number of variables in the ambient ring. */
  size_t getVarCount() const {return _varCount;}

  /** Returns \f$I\f$ for a slice \f$(I,S,q)\f$. There is no non-const
   getIdeal() because Slice caches properties of the ideal, and it is
   not possible to efficiently track changes performed directly on the
   ideal. To compensate for this, Slice provides a subset of the
   mutable interface of Ideal which allows to manipulate the ideal.
  */
  const Ideal& getIdeal() const {return _ideal;}

  /** Returns \f$S\f$ for a slice \f$(I,S,q)\f$. */
  Ideal& getSubtract() {return _subtract;}

  /** Returns \f$S\f$ for a slice \f$(I,S,q)\f$. */
  const Ideal& getSubtract() const {return _subtract;}

  /** Returns \f$q\f$ for a slice \f$(I,S,q)\f$. */
  Term& getMultiply() {return _multiply;}

  /** Returns \f$q\f$ for a slice \f$(I,S,q)\f$. */
  const Term& getMultiply() const {return _multiply;}

  /** Returns the least common multiple of the generators of
   getIdeal(). The lcm is stored and is only recomputed once after
   each time the ideal changes. The lcm is always needed after each
   change, e.g. to detect if the slice is a base case slice, so
   calling this method should be regarded as an inexpensive operation.
  */
  const Term& getLcm() const;

  /** Write a text representation of this object to file in a format
   appropriate for debugging. */
  void print(FILE* file) const;

  //@}

  /** @name Mutators */
  //@{

  /** Returns true if this slice is a base case slice, and in that
   case produces output in a derivative-specific way. If simplified is
   true, then the slice must be fully simplified when calling
   baseCase(), while otherwise there is no such requirement.
  */
  virtual bool baseCase(bool simplified) = 0;

  /** Performs a deep copy of slice into this object. */
  virtual Slice& operator=(const Slice& slice) = 0;

  /** Resets this slice to \f$(\ideal 0, \ideal 0, 1)\f$ in an ambient
   polynomial ring of varCount variables.
  */
  void resetAndSetVarCount(size_t varCount);

  /** Clears getIdeal() and getSubtract() and does not change
   getMultiply(). This is useful to induce this slice to be clearly a
   trivial base case slice, and to clear memory in preparation for
   reusing this slice later without having to construct a new
   Slice. getMultiply() is left unchanged since changing it is
   unnecessary for these purposes.
  */
  void clearIdealAndSubtract();

  /** Calls Ideal::singleDegreeSort on getIdeal(). */
  void singleDegreeSortIdeal(size_t var);

  /** Calls Ideal::insert on getIdeal(). */
  void insertIntoIdeal(const Exponent* term);

  /** Sets this object to the inner slice according to pivot. To be
   precise, the slice \f$(I,S,q)\f$ is replaced by \f$(I:p,S:p,qp)\f$
   where \f$p\f$ is the pivot, and the slice is then normalized (see
   \ref normalize).

   Returns true if any of the colon operations \f$I:p\f$ and \f$S:p\f$
   were non-trivial in the sense that it changed the support of any
   minimal generator.
  */
  virtual bool innerSlice(const Term& pivot);

  /** Sets this object to the outer slice according to pivot. To be
   precise, the slice \f$(I,S,q)\f$ is replaced by \f$(I,S+\ideal
   p,q)\f$ where \f$p\f$ is the pivot, and the slice is then
   normalized (see \ref normalize).

   Note that if pivot is a pure power, then pivot is not actually
   inserted into \f$S\f$ since doing so has no effect on the content
   after the normalization.
  */
  virtual void outerSlice(const Term& pivot);

  /** Removes those generators of getIdeal() that are strictly
   divisible by some generator of getSubtract(). Note that this does
   not change the content of the slice. Returns true if any generators
   were removed. See ::strictlyDivides for the definition of strict
   divisibility.
  */
  bool normalize();

  /** Simplifies this object such that \ref normalize, \ref
   pruneSubtract, \ref removeDoubleLcm and \ref applyLowerBound all
   return false. It is a precondition that the slice is already
   normalized.

   @todo what is removedoublelcm?
  */
  virtual void simplify() = 0;

  /** Like \ref simplify, except that only one simplification step is
   performed. If the return value is true, then the Slice may not be
   fully simplified yet. Iterating simplifyStep() has the same
   result as calling simplify(), though the performance
   characteristics can be worse.

   @todo Is this method actually used, and does it return true iff
    this object changed?
  */
  virtual bool simplifyStep() = 0;

  //@}

 protected:
  /** Set this object to be the projection of slice according to
   projection. I.e. each of getIdeal(), getSubtract() and
   getMultiply() are projected.
  */
  void setToProjOf(const Slice& slice, const Projection& projection);

  /** Simultaneously set the value of this object to that of slice and
   vice versa. This is an inexpensive operation because no copy is
   necessary.
  */
  void swap(Slice& slice);

  /** Removes those generators of subtract that do not strictly divide
   the lcm of getIdeal(), or that belong to getIdeal(). Note that
   removing these generators does not change the content. Returns true
   if any generators were removed.
  */
  bool pruneSubtract();

  /** Calculates a lower bound on the content of the slice using \ref
   getLowerBound and calls innerSlice with that lower bound. Note that
   this does not change the content of the slice. This is repeated
   until a fixed point is reached. Returns false if no minimal
   generator of getIdeal() or getSubtract() has had their support
   changed or if a trivial base case is detected.

   @todo Rename lower bound to divisor.
  */
  bool applyLowerBound();

  /** Calculates a lower bound that depends on var. To be precise, the
   lower bound that is calculated is \f[
     \frac{1}{x_i}\gcd(\min(I)\cap\ideal{x_i})
   \f] where \f$i\f$ is var. Note that the real functionality is
   slightly more sophisticated. Returns false and does not set bound
   if a base case is detected (a base case is not guaranteed to be
   detected).

   @todo rename lower bound to divisor.
   @todo describe how the real functionality is slightly more
    sophisticated.
  */
  virtual bool getLowerBound(Term& bound, size_t var) const = 0;

  /** The \f$I\f$ of a slice \f$(I,S,q)\f$. */
  Ideal _ideal;

  /** The \f$S\f$ of a slice \f$(I,S,q)\f$. */
  Ideal _subtract;

  /** The \f$q\f$ of a slice \f$(I,S,q)\f$. */
  Term _multiply;

  /** The number of variables in the ambient polynomial ring. */
  size_t _varCount;

  /** The lcm of getIdeal() if _lcmUpdated is true, and otherwise the
   value is undefind. _lcm will always have the correct number of
   variables, even when _lcmUpdated is false.

   This member variable is mutable since it has to be updated by \ref
   getLcm in case _lcmUpdated is false, but \ref getLcm should be
   const.
  */
  mutable Term _lcm;

  /** Indicates whether _lcm is correct.

   This member variable is mutable since it has to be updated by \ref
   getLcm in case _lcmUpdated is false, but \ref getLcm should be
   const.
  */
  mutable bool _lcmUpdated;

  /** A hint that starting simplification through a lower bound at the
   variable indicated by _lowerBoundHint is likely to yield a
   simplification, or at least more likely than a random other
   variable. The point of this is to detect variables that can be
   simplified sooner in order to speed up simplification.
  */
  size_t _lowerBoundHint;
};

#endif
