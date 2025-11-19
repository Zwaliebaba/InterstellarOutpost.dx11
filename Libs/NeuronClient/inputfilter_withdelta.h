#ifndef INCLUDED_INPUTFILTER_WITHDELTA_H
#define INCLUDED_INPUTFILTER_WITHDELTA_H

#include "auto_vector.h"
#include "inputfilter.h"
#include "inputfilterspec.h"

class InputFilterWithDelta : public InputFilter
{
  auto_vector<const InputFilterSpec> m_specs;
  auto_vector<InputDetails> m_oldDetails;
  auto_vector<InputDetails> m_details;

  protected:
    void registerDeltaID(InputFilterSpec& spec);

    const InputDetails& getOldDetails(filterspec_id_t id) const;
    const InputDetails& getOldDetails(const InputFilterSpec& spec) const;

    const InputDetails& getDetails(filterspec_id_t id) const;
    const InputDetails& getDetails(const InputFilterSpec& spec) const;

    // Subclasses must implement this
    virtual void calcDetails(const InputFilterSpec& spec, InputDetails& details) = 0;

    // Preferably override the first of these, unless you need to use more
    // filterspec information, in which case you should probably override both
    virtual int getDelta(filterspec_id_t id, InputDetails& delta) const;
    virtual int getDelta(const InputFilterSpec& spec, InputDetails& delta) const;

    // After this, the current details will be invalid
    void ageDetails();

  public:
    void Advance() override;
};

#endif // INCLUDED_INPUTFILTER_WITHDELTA_H
