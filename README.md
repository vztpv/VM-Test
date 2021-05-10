# VM-Test

# ToDo (need....)
    1. Log Class (file or console output)(Error, General, Notice, Warnning)
    2. long long, long double, std::string_view, bool, and struct Workspace { clau_parser::UserType* ut; long long idx; }
    3. ClauParser + line info + my own smart ptr?
# ClauScript++ 구상
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

    eu4 = {

    }
    eu4 = {

    }
    eu4 = ok 

    Event = {
        id = main	# ClauScript Main - deleted..

        # load data from file.
        $find = { /./eu4/ }  # /dir/   /file

        $while { $not_empty = { /$return_value } } {
            $load_data = { /$return_value "C:\Users\vztpv\Desktop\Clau\ClauParser\ClauParser\input.eu4" }

            $call = { id = iterate workspace = /$return_value/$enter event = test }

            $pop_front = { /$return_value } # /$return_value/$pop_front
        }
    }

    Event = {
        id = iterate

        $parameter = { workspace event } # recursive?

        # $ <- 데이터영역?에서 $로 시작하지않는다. - 조건?

        $while { $COMP< = { /$parameter.workspace/$get_idx /$parameter.workspace/$get_size } } {
            $call = { id = $parameter.event iter = /$parameter.workspace/$get_now } # $get_now = { } -> pair of UserType*, and long long

            $if { /$parameter.workspace/$is_group } {
                $call = { id = iterate workspace = /$parameter.workspace/$clone/$enter } # $enter = { } -> pair of UserType*, and long long
            }
            $set_idx = { /$parameter.workspace $add = { /$parameter.workspace/$get_idx 1 } }
        }
    }

    Event = {
        id = test

        $parameter = { iter }	

        $if { 
                $AND_ALL = { 
                    $NOT = { $is_quoted_str = { /$parameter.iter/$get_name } }			
                    $COMP> = { /$parameter.iter/$get_name 1444.1.1 }
                }
            } {
            $set_name = { /$parameter.iter 1444.1.1 }
        }
        $else if { 
                $AND_ALL = { 
                    $is_quoted_str = { /$parameter.iter/$get_name }
                    $COMP> = { $remove_quoted = { /$parameter.iter/$get_name } 1444.1.1 }
                }
            } {
            $set_name = { /$parameter.iter "1444.1.1" }
        }

        $if { 
                $AND_ALL = {
                    /$parameter.iter/$is_item
                    $NOT = { $is_quoted_str = { /$parameter.iter/$get_value } }
                    $COMP> = { /$parameter.iter/$get_value 1444.1.1 }
                }
            } {
            $set_value = { /$parameter.iter 1444.1.1 }
        }
        $else if { 
                $AND_ALL = { 
                    /$parameter.iter/$is_item
                    $is_quoted_str = { /$parameter.iter/$get_name }
                    $COMP> = { $remove_quoted = { /$parameter.iter/$get_name } 1444.1.1 }
                }
            } {
            $set_value = { /$parameter.iter "1444.1.1" }
        }
    }



