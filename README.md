# VM-Test

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

        Event = {
            id = main

            $call = { id = iterate event = test }
        }

        Event = {
            id = iterate

            $parameter = { workspace = $root event } # recursive?

            # $ <- 데이터영역?에서 $로 시작하지않는다. - 조건?

            $if { $COMP< = { /$parameter.workspace/$get_idx /$parameter.workspace/$get_size } } {
                $call = { id = $parameter.event iter = /$parameter.workspace/$get_now } # $get_now = { } -> pair of UserType*, and long long

                $if { /$parameter.workspace/$is_group } {
                    $call = { id = iterate workspace = /$parameter.workspace/$enter } # $enter = { } -> pair of UserType*, and long long
                }
                $set_idx = { $add = { /$parameter.workspace/$get_idx 1 } }
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
                $set_name = { 1444.1.1 }
            }
            $else if { 
                    $AND_ALL = { 
                        $is_quoted_str = { /$parameter.iter/$get_name }
                        $COMP> = { $remove_quoted = { /$parameter.iter/$get_name } 1444.1.1 }
                    }
                } {
                $set_name = { "1444.1.1" }
            }

            $if { 
                    $AND_ALL = {
                        /$paraemter.iter/$is_item
                        $NOT = { $is_quoted_str = { /$parameter.iter/$get_value } }
                        $COMP> = { $remove_quoted = { /$parameter.iter/$get_value } 1444.1.1 }
                    }
                } {
                $set_value = { 1444.1.1 }
            }
            $else if { 
                    $AND_ALL = { 
                        /$parameter.iter/$is_item
                        $is_quoted_str = { /$parameter.iter/$get_name }
                        $COMP> = { $remove_quoted = { /$parameter.iter/$get_name } 1444.1.1 }
                    }
                } {
                $set_value = { "1444.1.1" }
            }
        }

