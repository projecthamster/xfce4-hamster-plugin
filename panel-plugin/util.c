#include "util.h"

fact *
fact_new(GVariant *in)
{
   //bzero(out, sizeof(fact));
   fact *out = g_new0(fact, 1);
   g_variant_get(in, "(iiissisasii)",
         &out->id,
         &out->startTime,
         &out->endTime,
         &out->description,
         &out->name,
         &out->activityId,
         &out->category,
         &out->tags,
         &out->date,
         &out->seconds
         );
   return out;
}

void
fact_free(fact *in)
{
   g_free(in->description);
   g_free(in->name);
   g_free(in->category);
   //g_free(in->tags); // TODO: fix leak
   g_free(in);
}
