#include "common.h"

AI_SHADER_NODE_EXPORT_METHODS(UserDataRampVMtd);

enum UserDataRampVParams
{
   p_input = 0,
   p_positions,
   p_sort_positions,
   p_values,
   p_interpolations,
   p_default_interpolation,
   p_default_value,
   p_abort_on_error
};

node_parameters
{
   AiParameterFlt("input", 0.0f);
   AiParameterStr(SSTR::positions, "");
   AiParameterBool(SSTR::sort_positions, false);
   AiParameterStr(SSTR::values, "");
   AiParameterStr(SSTR::interpolations, "");
   AiParameterEnum(SSTR::default_interpolation, 1, InterpolationTypeNames);
   AiParameterVec("default_value", 1.0f, 0.0f, 1.0f);
   AiParameterBool(SSTR::abort_on_error, false);
}

struct NodeData
{
   AtString positions;
   AtString values;
   AtString interpolations;
   InterpolationType defaultInterpolation;
   bool abortOnError;
   bool sortPositions;
};

node_initialize
{
   AiNodeSetLocalData(node, new NodeData());
}

node_update
{
   NodeData *data = (NodeData*) AiNodeGetLocalData(node);
   
   data->positions = AiNodeGetStr(node, SSTR::positions);
   data->values = AiNodeGetStr(node, SSTR::values);
   data->interpolations = AiNodeGetStr(node, SSTR::interpolations);
   data->defaultInterpolation = (InterpolationType) AiNodeGetInt(node, SSTR::default_interpolation);
   data->abortOnError = AiNodeGetBool(node, SSTR::abort_on_error);
   data->sortPositions = AiNodeGetBool(node, SSTR::sort_positions);
}

node_finish
{
   NodeData *data = (NodeData*) AiNodeGetLocalData(node);
   delete data;
}

static void Failed(AtShaderGlobals *sg, AtNode *node, const char *errMsg, bool abortOnError)
{
   if (abortOnError)
   {
      if (errMsg)
      {
         AiMsgError("[%suser_data_ramp_v] %s", PREFIX, errMsg);
      }
      else
      {
         AiMsgError("[%suser_data_ramp_v] Failed", PREFIX);
      }
   }
   else
   {
      if (errMsg)
      {
         AiMsgWarning("[%suser_data_ramp_v] %s. Use default value", PREFIX, errMsg);
      }
      else
      {
         AiMsgWarning("[%suser_data_ramp_v] Failed. Use default value", PREFIX);
      }
      sg->out.VEC = AiShaderEvalParamVec(p_default_value);
   }
}

shader_evaluate
{
   NodeData *data = (NodeData*) AiNodeGetLocalData(node);
   
   AtArray *p, *v, *i=0;
   
   if (!AiUDataGetArray(data->positions, &p))
   {
      Failed(sg, node, "Positions user attribute not found or not an array", data->abortOnError);
      return;
   }
   
   if (p->type != AI_TYPE_FLOAT)
   {
      Failed(sg, node, "Positions user attribute must be an array of floats", data->abortOnError);
      return;
   }
   
   if (!AiUDataGetArray(data->values, &v))
   {
      Failed(sg, node, "Values user attribute not found or not an array", data->abortOnError);
      return;
   }
   
   if (v->type != AI_TYPE_VECTOR && v->type != AI_TYPE_POINT)
   {
      Failed(sg, node, "Values user attribute must be an array of vectors/points", data->abortOnError);
      return;
   }
   
   if (v->nelements != p->nelements)
   {
      Failed(sg, node, "Positions and values array size mismatch", data->abortOnError);
      return;
   }
   
   if (AiUDataGetArray(data->interpolations, &i))
   {
      if (i->type != AI_TYPE_ENUM && i->type != AI_TYPE_INT)
      {
         Failed(sg, node, "Interpolations user attribute must be an array of enums or ints", data->abortOnError);
         return;
      }
      
      if (i->nelements != p->nelements)
      {
         Failed(sg, node, "Positions and interpolations array size mismatch", data->abortOnError);
         return;
      }
   }
   
   if (data->sortPositions)
   {
      unsigned int *s = (unsigned int*) AiShaderGlobalsQuickAlloc(sg, p->nelements * sizeof(unsigned int));
      SortPositions(p, s);
      EvalVectorRamp(p, v, i, data->defaultInterpolation, s, AiShaderEvalParamFlt(p_input), sg->out.VEC);
   }
   else
   {
      EvalVectorRamp(p, v, i, data->defaultInterpolation, AiShaderEvalParamFlt(p_input), sg->out.VEC);
   }
}