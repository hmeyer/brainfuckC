#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "token.hpp"
#include <vector>
#include <unordered_map>


namespace nbf {

class Transformer {          
public:                               
  Transformer(std::string_view code);
  std::string transform();
private:
  std::string moveToVar(const Variable& var);
  std::string moveTo(int tape);

  std::vector<Token> tokens_;                    
  std::unordered_map<std::string, int> var2tape_;
  std::unordered_map<std::string, int> var2size_;
  int max_tape_ = 0;
  int current_tape_ = 0;

};

}  // namespace nbf

#endif  // TRANSFORMER_H