# VM-Test

# ToDo (need....)
    1. Log Class (file or console output)(Error, General, Notice, Warnning)
    2. long long, long double, std::string, bool, and struct Workspace { clau_parser::UserType* ut; long long idx; },
        and vector of ~~
    4. ClauParser + line info + my own smart ptr? (shared, weak?)
# ClauScript++ 구상
    $func = { a } 
    @a@$func
    
    $if, $else, # 조건문 
    $while, # 반복문
    $call, # 사용자 정의 이벤트 호출
    $return,
    $print,
       #  +, -, *, /, %
       # string operation.
       # edit_mode, shell_mode
       # lint, query
       # from clau_parser::Reader, Writer.? - set_idx, enter, is_group, is_item, back?quit?, 
       #                            get_name, get_value, set_name, set_value, get_idx, get_size?
       #                          default idx 0? new_item, new_group.? insert?(anywhere?)
       #                                new_group, new_item, remove_group, remove_item
       # tobool4?
       # find usertypes by /./dir1/dir2/ ...
       # get id for usertype ..
       # change now usertype?
       # get now usertype?
       # make_map ? 
       # return_value
       
    # Events, # id = push_back id = iterate? id = if id = make id = assign ...
              # id = while id = for id = call ... 
       # make_var_int, get_var_int, set_var_int <- var_id?


# ClauScript++ Example...

    # ToDo - end of function.?  $find = { @eu4 } FUNC_FIND DIR_START DIR END_DIR (END_FUNC?)
    # ToDo - $get = { /./test } #"eu4"
    # ToDo - @/./test@$get
    # Done - @eu4@$find  # <- now ok.

    test = "eu4"

    Test = {
        eu4 = {

        }
    }

    Event = {
        id = main

    	$print = { @/./test@$get }
        $print = { @1@2@$add@4@$add }
        $print = { \n } 

        # load data from file.
        #$find = { /Test/eu4 }  

        @/Test/eu4@$find 

        $load_data = { $return_value = { } "C:\Users\vztpv\Desktop\Clau\ClauParser\ClauParser\input.eu4" }

        $call = { id = iterate workspace = @$return_value@$clone  event = test } # @$return_value ??

        # @a@$func <- no ok.
        # $func2 = { @a@$func } # <- ok.

    }

    Event = {
        id = iterate

        $parameter = { workspace event } # recursive?

        # $ <- 데이터영역?에서 $로 시작하지않는다. - 조건?

        $set_idx = { @$parameter.workspace 0 }

        $while { $COMP< = { $get_idx = { @$parameter.workspace } $get_size = { @$parameter.workspace } } } 
        {	
            $call = { id = $parameter.event 
                iter = @$parameter.workspace
                name = @$parameter.workspace@$get_name 
                value = @$parameter.workspace@$get_value
                is_user_type = @$parameter.workspace@$is_group
            } 

            $if { @$parameter.workspace@$is_group } {
                @$parameter.workspace@$enter
                $call = { id = iterate workspace = @$parameter.workspace@$clone event = $parameter.event } # $enter = { } -> pair of UserType*, and long long
                @$parameter.workspace@$quit
            }
            $set_idx = { @$parameter.workspace $add = { @$parameter.workspace@$get_idx 1 } }
        }
    }

    Event = {
        id = test

        $parameter = { iter name value is_user_type }	

        $if { 
                $AND = { 
                    $NOT = { $is_quoted_str = { $parameter.name } }			
                    $NOT = { $is_quoted_str = { $parameter.name } }			
                    # $COMP> = { $parameter.name 1444 }
                }
            } {

            #@$parameter.name@$print

            #$print = { $parameter.name } 

            $set_name = { @$parameter.iter $parameter.name }
        }
        $if { 
                $AND = { 
                    $is_quoted_str = { $parameter.name }
                    $is_quoted_str = { $parameter.name }

                    #$COMP> = { $remove_quoted = { $parameter.name } 1444 }
                }
            } {

            #@$parameter.name@$print

            #$print = { $parameter.name } 

            $set_name = { @$parameter.iter $remove_quoted = { $parameter.name} }
        }

        $if { 
                $AND = {
                    $NOT = { $parameter.is_user_type }
                    #$AND_ALL = {#
                        $NOT = { $is_quoted_str = { $parameter.value } }
                        #$COMP> = {  $parameter.value 1444 }
                    #}
                }
            } {

            #@:@$print
            #@$parameter.value@$print
            #@\n@$print

            #$print = { : }
            #$print = { $parameter.value } 
            #$print = { \n }

            $set_value = { @$parameter.iter $parameter.value }
        }
        $if { 
                $AND = { 
                    $NOT = { $parameter.is_user_type }
                    #$AND_ALL = { 
                        $is_quoted_str = { $parameter.value }
                        #$COMP> = { $remove_quoted = { $parameter.value } 1444 }
                    #}
                }
            } {

            #@:@$print
            #@$parameter.value@$print
            #@\n@$print

            #$print = { : }
            #$print = { $parameter.value } 
            #$print = { \n }

            $set_value = { @$parameter.iter $remove_quoted = { $parameter.value } }
        }
    }

