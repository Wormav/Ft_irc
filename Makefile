NAME			:= ircserv

# Répertoires
SRCS_DIR		:= src
OBJS_DIR		:= .obj
INCS_DIR		:= includes

# Compilation
CXX				:= c++
FLAGXX			:= -std=c++98 -Wall -Wextra -Werror -g
IFLAGS			:= -I $(INCS_DIR)

# Sources
SRCS			:=	main.cpp \
					Server.cpp \
					Channel.cpp \
					User.cpp \
					Command.cpp \


SRCS			:= $(addprefix $(SRCS_DIR)/, $(SRCS))
OBJS			:= $(SRCS:%.cpp=$(OBJS_DIR)/%.o)

# Commandes
RM				:= rm -rf
DIR_UP			= mkdir -p $(@D)

# Couleurs et emojis
RED				:= \033[1;31m
GREEN			:= \033[1;32m
YELLOW			:= \033[1;33m
RESET			:= \033[0m

SUCXXESS_EMOJI	:= ✅
CLEAN_EMOJI		:= 🗑️
BUILD_EMOJI		:= 🛠️

################################################################################

all: $(OBJS_DIR) $(NAME)

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)/$(SRCS_DIR)

$(NAME): $(OBJS)
	@echo "$(YELLOW)$(BUILD_EMOJI)  Compilation in progress...$(RESET)"
	@$(CXX) $(FLAGXX) $(OBJS) -o $@
	@echo "$(GREEN)$(SUCXXESS_EMOJI) Compilation complete !$(RESET)"

$(OBJS_DIR)/%.o: %.cpp
	@$(DIR_UP)
	@echo "$(YELLOW)$(BUILD_EMOJI)  Compilation of $<...$(RESET)"
	@$(CXX) $(FLAGXX) $(IFLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJS_DIR)
	@echo "$(RED)$(CLEAN_EMOJI)  Objects deleted!$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(RED)$(CLEAN_EMOJI)  Executable deleted!$(RESET)"

re: fclean all

.PHONY: all clean fclean re
