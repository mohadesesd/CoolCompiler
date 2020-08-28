Class List inherits IO { 
  isEmpty() : Bool { { abort(); true; } };
  sort() : List {{ abort(); new List; }};
  insert(i : Int) : List { { abort(); new List; } };
  print_(counter : Int) : Object { abort() };
};

Class ListNode inherits List {
  val : Int;  
  next : List; 
  is_Empty :Bool <- true;
  isEmpty() : Bool { is_Empty};
  init__(head : Int, tail : List) :ListNode {
    {
      is_Empty <- false;
      val <- head;
      next <- tail;
      self;
    }
  };  
    sort() : List { if(isEmpty()) then self
              else (next.sort()).insert(val)
                    fi };

  insert(data : Int) : List {
    if(true = isEmpty()) then (new ListNode).init__(data,self)
    else
      if data < val then
          (new ListNode).init__(data,self)
      else
          (new ListNode).init__(val,next.insert(data))
      fi
    fi
  };
  print_(counter : Int) : Object {
    {
        if(true = isEmpty()) then true
        else{
         if (1 = counter) then out_string("The sorted list is:\n")
         else true
         fi;
         out_string("[");
         out_int(counter);
         out_string("]  ");
         out_int(val);
         out_string("\n");
         counter <- counter + 1; 
         next.print_(counter);
        }
        fi;
    }
  };
};

Class Main inherits IO {

  list : List;
  get_list() : List {
      {
     list <- new ListNode;
    (let length :Int <- in_int(), counter : Int <- 1 in
       while counter <= length
       loop 
         {
                 list <- (new ListNode).init__(in_int(),list);
           counter <- counter + 1;
         } 
       pool
    );
    list;
      }
  };    

  main() : Object {
     {
           out_string("Enter the size of your input list and then Enter your list element by element:\n");
           get_list().sort().print_(1);
     }
  };
};
