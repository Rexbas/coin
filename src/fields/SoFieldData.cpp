/**************************************************************************\
 *
 *  Copyright (C) 1998-1999 by Systems in Motion.  All rights reserved.
 *
 *  This file is part of the Coin library.
 *
 *  This file may be distributed under the terms of the Q Public License
 *  as defined by Troll Tech AS of Norway and appearing in the file
 *  LICENSE.QPL included in the packaging of this file.
 *
 *  If you want to use Coin in applications not covered by licenses
 *  compatible with the QPL, you can contact SIM to aquire a
 *  Professional Edition license for Coin.
 *
 *  Systems in Motion AS, Prof. Brochs gate 6, N-7030 Trondheim, NORWAY
 *  http://www.sim.no/ sales@sim.no Voice: +47 22114160 Fax: +47 67172912
 *
\**************************************************************************/

/*!
  \class SoFieldData Inventor/fields/SoFieldData.h
  \brief The SoFieldData class is a container for a prototype set of fields.
  \ingroup fields

  This class is instantiated once for each class of objects which use
  fields, and which needs to be able to import and export them.

  Each field of a class is stored with the name it has been given
  within its "owner" class and a pointer offset to the dynamic
  instance of the field itself.

  Enumeration sets are stored with (name value) pairs, to make it
  possible to address, read and save enum type fields by name.

  It is unlikely that application programmers should need to use any
  of the methods of this class directly.

  \sa SoField, SoFieldContainer */

/*�
  Some methods related to reading VRML 2 files are missing.
 */

#include <Inventor/fields/SoFieldData.h>
#include <Inventor/SbName.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoFieldContainer.h>
#include <Inventor/misc/SoBasic.h> // COIN_STUB()
#include <ctype.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

static const char OPEN_BRACE_CHAR = '[';
static const char CLOSE_BRACE_CHAR = ']';
static const char VALUE_SEPARATOR_CHAR = ',';


/// Internal classes, start /////////////////////////////////////////////////

class SoFieldEntry {
public:
  SoFieldEntry(const char * n, int offset) : name(n), ptroffset(offset) { }
  // Copy constructors.
  SoFieldEntry(const SoFieldEntry * fe) { this->copy(fe); }
  SoFieldEntry(const SoFieldEntry & fe) { this->copy(&fe); }

  SbName name;
  int ptroffset;

private:
  void copy(const SoFieldEntry * fe)
    {
      this->name = fe->name;
      this->ptroffset = fe->ptroffset;
    }
};

class SoEnumEntry {
public:
  SoEnumEntry(const SbName & name) : nameoftype(name) { }
  // Copy constructors.
  SoEnumEntry(const SoEnumEntry * ee) { this->copy(ee); }
  SoEnumEntry(const SoEnumEntry & ee) { this->copy(&ee); }

  SbName nameoftype;
  SbList<SbName> names;
  SbList<int> values;

private:
  void copy(const SoEnumEntry * ee)
    {
      this->nameoftype = ee->nameoftype;
      this->names = ee->names;
      this->values = ee->values;
    }
};

/// Internal classes, end ///////////////////////////////////////////////////



/*!
  Default constructor.
 */
SoFieldData::SoFieldData(void)
{
}

/*!
  Copy constructor.
 */
SoFieldData::SoFieldData(const SoFieldData & fd)
{
  this->copy(&fd);
}

/*!
  Copy constructor taking a pointer value as an argument. Handles \c
  NULL pointers by behaving like the default constructor.
*/
SoFieldData::SoFieldData(const SoFieldData * fd)
{
  if (fd) this->copy(fd);
}

/*!
  Constructor. Takes an indication on the number of fields which
  should be stored.
 */
SoFieldData::SoFieldData(int /* numfields */)
{
  // Ignore the argument, I don't think there's any point in doing
  // optimization here. 19991231 mortene.
}

/*!
  Destructor.
 */
SoFieldData::~SoFieldData()
{
  this->freeResources();
}

// Empties internal lists, while deallocating the memory used for the
// entries.
void
SoFieldData::freeResources(void)
{
  for (int i=0; i < this->fields.getLength(); i++) delete this->fields[i];
  this->fields.truncate(0);

  for (int j=0; j < this->enums.getLength(); j++) delete this->enums[j];
  this->enums.truncate(0);
}

/*!
  Add a new field to our internal list.

  The \a name will be stored along with an pointer offset between \a
  base and \a field, which will be valid for all instances of the
  class type of \a base.
*/
void
SoFieldData::addField(SoFieldContainer * base, const char * name,
                      const SoField * field)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoFieldData::addField",
                         "class: ``%s'', field: ``%s''",
                         base->getTypeId().getName().getString(),
                         name);
#endif // debug

  char * vbase = (char *)base;
  char * vfield = (char *)field;
  this->fields.append(new SoFieldEntry(name, vfield - vbase));
}

/*!
  FIXME: write doc
 */
void
SoFieldData::overlay(SoFieldContainer * /* to */,
                     const SoFieldContainer * /* from */,
                     SbBool /* copyConnections */) const
{
  COIN_STUB();
}

/*!
  Returns number of fields contained within this instance.
 */
int
SoFieldData::getNumFields(void) const
{
  return this->fields.getLength();
}

/*!
  Returns the name of the field at \a index.
 */
const SbName &
SoFieldData::getFieldName(int index) const
{
  return this->fields[index]->name;
}

/*!
  Returns a pointer to the field at \a index within the \a object
  instance.
 */
SoField *
SoFieldData::getField(const SoFieldContainer * object, int index) const
{
  char * fieldptr = (char *)object;
  fieldptr += this->fields[index]->ptroffset;
  return (SoField *)fieldptr;
}

/*!
  Returns the internal index value of \a field in \a fc. If \a field
  is not part of \a fc, return -1.
*/
int
SoFieldData::getIndex(const SoFieldContainer * fc, const SoField * field) const
{
  char * vbase = (char *)fc;
  char * vfield = (char *)field;
  int ptroffset = vfield - vbase;

  for (int i=0; i < this->fields.getLength(); i++)
    if (this->fields[i]->ptroffset == ptroffset) return i;

  return -1;
}

/*!
  Either adds a new enum set (with an initial member), or adds a new value
  member to an existing enum set.
*/
void
SoFieldData::addEnumValue(const char * enumname, const char * valuename,
                          int value)
{
  SoEnumEntry * e = NULL;

  for (int i=0; !e && (i < this->enums.getLength()); i++) {
    if (this->enums[i]->nameoftype == enumname) e = this->enums[i];
  }

  if (e == NULL) {
    e = new SoEnumEntry(enumname);
    this->enums.append(e);
  }

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoFieldData::addEnumValue",
                         "enumname: %s, valuename: %s, value: %d",
                         enumname, valuename, value);
#endif // debug

  assert(e->names.find(valuename) == -1);
  e->names.append(valuename);
  // Note that an enum can have several names mapping to the same
  // value. 20000101 mortene.
  e->values.append(value);
}

/*!
  Returns the \a names and \a values of enumeration entry with the \a
  enumname. The number of (name value) pairs available in the
  enumeration is returned in \a num.
*/
void
SoFieldData::getEnumData(const char * enumname, int & num,
                         const int *& values, const SbName *& names)
{
  num = 0;
  values = NULL;
  names = NULL;

  for (int i=0; i < this->enums.getLength(); i++) {
    SoEnumEntry * e = this->enums[i];
    if (e->nameoftype == enumname) {
      num = e->names.getLength();
      if (num) {
        assert(e->names.getLength() == e->values.getLength());
        names = e->names.constArrayPointer();
        values = e->values.constArrayPointer();
      }
      return;
    }
  }
}

/*!
  Read field data from the \a in stream for fields belonging to \a
  object. Returns \c TRUE if everything went ok, or \c FALSE if any
  error conditions occurs.

  \a erroronunknownfield decides whether or not \c FALSE should be
  returned if a name identifier not recognized as a fieldname of \a
  object is encountered. Note that \a erroronunknownfield should be \c
  FALSE if \a object is a container with child objects, otherwise the
  code will fail upon the first child name specification.

  If \a notbuiltin is \c TRUE on return, \a object is an unknown node
  or engine type. Unknown nodes are recognized by the \c fields
  keyword first in their file format definition, and unknown engines
  by the \a inputs keyword.

*/
SbBool
SoFieldData::read(SoInput * in, SoFieldContainer * object,
                  SbBool erroronunknownfield, SbBool & notbuiltin) const
{
  notbuiltin = FALSE;

  if (in->isBinary()) {
    int numfields;
    if (!in->read(numfields)) {
      SoReadError::post(in, "Premature EOF");
      return FALSE;
    }

    // FIXME: invalid check? (same field may be written several
    // times?) 19990711 mortene.
    if (numfields > this->fields.getLength()) {
      SoReadError::post(in, "Invalid number of fields: %d", numfields);
      return FALSE;
    }

    if (numfields == 0) return TRUE;

    for (int i=0; i < numfields; i++) {
      SbName fieldname;
      if (!in->read(fieldname, TRUE) || !fieldname) {
        SoReadError::post(in, "Couldn't read field number %d", i);
        return FALSE;
      }

#if COIN_DEBUG && 0 // debug
      SoDebugError::postInfo("SoFieldData::read",
                             "fieldname: '%s'", fieldname.getString());
#endif // debug
      SbBool foundname;
      if (!this->read(in, object, fieldname, foundname)) return FALSE;

      // FIXME: handle case for binary format. 19990711 mortene.
      assert(foundname && "FIXME: doesn't work in binary mode yet");
    }
  }
  else { // ASCII format.
    SbBool firstidentifier = TRUE;
    while (TRUE) {
      SbName fieldname;
      if (!in->read(fieldname, TRUE)) return TRUE; // Terminates loop on "}"

      // This should be caught in SoInput::read(SbName, SbBool).
      assert(fieldname != "");

      SbBool foundname;
      if (!this->read(in, object, fieldname, foundname) && foundname)
        return FALSE;

      if (!foundname) {
        // User extension node with explicit field definitions.
        if (firstidentifier && fieldname == "fields") {
          notbuiltin = TRUE;
          if (!this->readFieldDescriptions(in, object, 0)) return FALSE;
        }
        // User extension engine with explicit input field definitions.
        else if (firstidentifier && fieldname == "inputs") {
          notbuiltin = TRUE;
          // FIXME: read input defs and inputs (and output
          // defs?). 20000102 mortene.
          COIN_STUB();
          return FALSE;
        }
        else if (erroronunknownfield) {
          SoReadError::post(in, "Unknown field \"%s\"", fieldname.getString());
          return FALSE;
        }
        else {
          in->putBack(fieldname.getString());
          return TRUE;
        }
      }
      firstidentifier = FALSE;
    }
  }

  return TRUE;
}

/*!
  Find field \a fieldname in \a object, and if it is available, sets
  \a foundname to \c TRUE and tries to read the field specification
  from \a in. If \a foundname is returned as \c TRUE, the return value
  says whether or not the field specification could be read without
  any problems.

  If \a fieldname is not part of \a object, returns \c FALSE with \a
  foundname also set to \c FALSE.
*/
SbBool
SoFieldData::read(SoInput * in, SoFieldContainer * object,
                  const SbName & fieldname, SbBool & foundname) const
{
  for (int i = 0; i < this->fields.getLength(); i++) {
    if (fieldname == this->getFieldName(i)) {
      foundname = TRUE;
      return this->getField(object, i)->read(in, fieldname);
    }
  }

  foundname = FALSE;
  return FALSE;
}

/*!
  FIXME: write doc
 */
void
SoFieldData::write(SoOutput * /* out */,
                   const SoFieldContainer * /* object */) const
{
  COIN_STUB();
}

/*!
  Copy contents of \a src into this instance.

  If there was any data set up in this instance before the method was
  called, the old data is removed.
 */
void
SoFieldData::copy(const SoFieldData * src)
{
  this->freeResources();

  int i;
  for (i=0; i < src->fields.getLength(); i++) {
    this->fields.append(new SoFieldEntry(src->fields[i]));
  }
  for (i=0; i < src->enums.getLength(); i++) {
    this->enums.append(new SoEnumEntry(src->enums[i]));
  }
}

/*!
  FIXME: write doc
 */
SbBool
SoFieldData::isSame(const SoFieldContainer * c1,
                    const SoFieldContainer * c2) const
{
  if (c1 == c2) return TRUE;

  COIN_STUB();
  return FALSE;
}

/*!
  Reads a set of field specifications from \a in for an unknown nodeclass type,
  in the form "[ FIELDCLASS FIELDNAME, FIELDCLASS FIELDNAME, ... ]".

 */
SbBool
SoFieldData::readFieldDescriptions(SoInput * in, SoFieldContainer * object,
                                   int /* numdescriptionsexpected */) const
{
  // These two macros are convenient for reading with error detection.
#define READ_CHAR(c) \
    if (!in->read(c)) { \
      SoReadError::post(in, "Premature end of file"); \
      return FALSE; \
    }

#define READ_NAME(n) \
    if (!in->read(n, TRUE)) { \
      SoReadError::post(in, "Premature end of file"); \
      return FALSE; \
    }


  // Binary format.
  if (in->isBinary()) {
    COIN_STUB();
    return FALSE;
  }
  // ASCII format.
  else {
    char c;
    READ_CHAR(c);
    if (c != OPEN_BRACE_CHAR) {
      SoReadError::post(in, "Expected '%c', got '%c'", OPEN_BRACE_CHAR, c);
      return FALSE;
    }

    while (TRUE) {
      SbName fieldtype;
      READ_NAME(fieldtype);

      SoType type = SoType::fromName(fieldtype.getString());
      if (type == SoType::badType()) {
        SoReadError::post(in, "Unknown field type '%s'", fieldtype.getString());
        return FALSE;
      }
      else if (!type.isDerivedFrom(SoField::getClassTypeId())) {
        SoReadError::post(in, "'%s' is not a field type", fieldtype.getString());
        return FALSE;
      }
      else if (!type.canCreateInstance()) {
        SoReadError::post(in, "Abstract class type '%s'", fieldtype.getString());
        return FALSE;
      }

      SbName fieldname;
      READ_NAME(fieldname);

#if COIN_DEBUG && 0 // debug
      SoDebugError::postInfo("SoFieldData::readFieldDescriptions",
                             "type: ``%s'', name: ``%s''",
                             fieldtype.getString(), fieldname.getString());
#endif // debug

      SbBool found = FALSE;
      for (int i=0; !found && (i < this->fields.getLength()); i++) {
        if (this->fields[i]->name == fieldname) found = TRUE;
      }
      if (!found) {
        // Cast away const -- ugly.
        SoFieldData * thisp = (SoFieldData *)this;
        thisp->addField(object, fieldname.getString(),
                        (const SoField *)type.createInstance());
      }

      READ_CHAR(c);

      if (c == VALUE_SEPARATOR_CHAR) {
        READ_CHAR(c);
        if (c == CLOSE_BRACE_CHAR) return TRUE;
        else in->putBack(c);
      }
      else if (c == CLOSE_BRACE_CHAR) return TRUE;
      else {
        SoReadError::post(in, "Expected '%c' or '%c', got '%c'",
                          VALUE_SEPARATOR_CHAR, CLOSE_BRACE_CHAR, c);
        return FALSE;
      }
    }
  }
  return TRUE;
}

/*!
  Write a set of field specifications to \a out for an unknown nodeclass type,
  in the form "[ FIELDCLASS FIELDNAME, FIELDCLASS FIELDNAME, ... ]".
 */
void
SoFieldData::writeFieldDescriptions(SoOutput * out,
                                    const SoFieldContainer * object) const
{
  // Binary format.
  if (out->isBinary()) {
    COIN_STUB();
  }
  // ASCII format.
  else {
    out->indent();
    out->write("fields [ ");

    for (int i=0; i < this->getNumFields(); i++) {
      out->write((const char *)(this->getField(object, i)->getTypeId().getName()));
      out->write(' ');
      out->write((const char *)(this->getFieldName(i)));
      out->write(", ");
    }

    out->write(" ]\n");
  }
}
