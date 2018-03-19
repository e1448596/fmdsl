
all_different(L):-fd_all_different(L).

all_different_0(_,[]).
all_different_0(V,[H|T]):- V#\=0 #==> V#\=H,all_different_0(V,T).
all_different_0([]).
all_different_0([H|T]):-all_different_0(H,T),all_different_0(T).

sum_eq(0,[]).
sum_eq(S,[A|L]):-sum_eq(SS,L),S#=(A+SS).

min_eq(M,[A]):-M#=A.
min_eq(M,[A|L]):-min_eq(MM,L),((A#<MM) #==> (M#=A)) #/\ ((A#>=MM) #==> (M#=MM)).

max_eq(M,[A]):-M#=A.
max_eq(M,[A|L]):-max_eq(MM,L),((A#>MM) #==> (M#=A)) #/\ ((A#=<MM) #==> (M#=MM)).

feature_labeling_opts([variable_method(most_constrained),value_method(max)]).
attr_labeling_opts([variable_method(most_constrained),value_method(min)]).

flatten_attr(_,[],_,_,[]).
flatten_attr(FeatureDict,[Feature-ValDict|AttrDict],Attribute,Default,[Var|L]):-memberchk(Attribute-Val,ValDict),
																			memberchk(Feature-Selected,FeatureDict),
																				(Selected #/\ (Var#=Val)) #\/ (#\Selected #/\ (Var#=Default)),
																					flatten_attr(FeatureDict,AttrDict,Attribute,Default,L),!.
flatten_attr(FeatureDict,[_|AttrDict],Attribute,Default,L):-flatten_attr(FeatureDict,AttrDict,Attribute,Default,L),!.

remove_attr_keys([],[]).
remove_attr_keys([_-Val|ValDict],[Val|Vals]):-remove_attr_keys(ValDict,Vals),!.

apply_filters(_,_,[]).
apply_filters(FeatureDict,AttrDict,[+Feature-[]=Vals|Filters]):-atom(Feature),
																		memberchk(Feature-1,FeatureDict),
																		memberchk(Feature-ValDict,AttrDict),
																		remove_attr_keys(ValDict,Vals),
																		apply_filters(FeatureDict,AttrDict,Filters),!.
apply_filters(FeatureDict,AttrDict,[Feature-[]=Var|Filters]):-atom(Feature),
																		memberchk(Feature-Selected,FeatureDict),
																		memberchk(Feature-ValDict,AttrDict),
																		remove_attr_keys(ValDict,Vals),
																		Var=Selected-Vals,
																		apply_filters(FeatureDict,AttrDict,Filters),!.
apply_filters(FeatureDict,AttrDict,[+Feature-Attribute=Val|Filters]):-atom(Feature),atom(Attribute),
																		memberchk(Feature-1,FeatureDict),
																		memberchk(Feature-ValDict,AttrDict),
																		memberchk(Attribute-Val,ValDict),
																		apply_filters(FeatureDict,AttrDict,Filters),!.
apply_filters(FeatureDict,AttrDict,[Feature-Attribute=Var|Filters]):-atom(Feature),atom(Attribute),
																		memberchk(Feature-Selected,FeatureDict),
																		memberchk(Feature-ValDict,AttrDict),
																		memberchk(Attribute-Val,ValDict),
																		Var=Selected-Val,
																		apply_filters(FeatureDict,AttrDict,Filters),!.
apply_filters(FeatureDict,AttrDict,[Attribute^Default=L|Filters]):-atom(Attribute),
																flatten_attr(FeatureDict,AttrDict,Attribute,Default,L),
																apply_filters(FeatureDict,AttrDict,Filters),!.
apply_filters(FeatureDict,AttrDict,[Attribute=L|Filters]):-atom(Attribute),
																flatten_attr(FeatureDict,AttrDict,Attribute,0,L),
																apply_filters(FeatureDict,AttrDict,Filters),!.
apply_filters(FeatureDict,AttrDict,[-Feature|Filters]):-atom(Feature),memberchk(Feature-0,FeatureDict),
															apply_filters(FeatureDict,AttrDict,Filters),!.
apply_filters(FeatureDict,AttrDict,[+Feature|Filters]):-atom(Feature),memberchk(Feature-1,FeatureDict),
															apply_filters(FeatureDict,AttrDict,Filters),!.

build_configuration(Product,Features,Attributes):-
	product_descriptor(Features,Attributes,FeatureDict,AttrDict),
	findall(Item,(member(Key-Selected,FeatureDict),
		(Selected=1->(memberchk(Key-ValDict,AttrDict)->Item=Key-ValDict;Item= +Key);Item= -Key)),Product).

product_valid_inner(Features,Attributes,Filters,Goal):-
					feature_labeling_opts(FLOpts),!,
					attr_labeling_opts(ALOpts),!,
					product_descriptor(Features,Attributes,FeatureDict,AttrDict),
					apply_filters(FeatureDict,AttrDict,Filters),
					product_hook(Features,Attributes,HookVars),
					call(Goal),
					fd_labeling(Features,FLOpts),fd_labeling(Attributes,ALOpts),fd_labeling(HookVars,ALOpts).

product_valid(Product,Filters,Goal):-product_model(1,Features,Attributes),
									product_valid_inner(Features,Attributes,Filters,Goal),
									build_configuration(Product,Features,Attributes).

product_count(Count,Filters,Goal):-findall(0,product_valid(_,Filters,Goal),L),length(L,Count).

features_core(Core,Filters,Goal):-var(Core),product_model(1,Features,Attributes),
											product_descriptor(Features,Attributes,FeatureDict,_),
											findall(F,(member(F-_,FeatureDict),\+product_valid_inner(Features,Attributes,[-F|Filters],Goal)),Core).

features_dead(Dead,Filters,Goal):-var(Dead),product_model(1,Features,Attributes),
											product_descriptor(Features,Attributes,FeatureDict,_),
											findall(F,(member(F-_,FeatureDict),\+product_valid_inner(Features,Attributes,[+F|Filters],Goal)),Dead).
											
features_optional(Opt,Filters,Goal):-var(Opt),features_dead(Dead,Filters,Goal),features_core(Core,Filters,Goal),
												product_descriptor(_,_,FeatureDict,_),
												findall(Feature,(member(Feature-_,FeatureDict),\+memberchk(Feature,Core),\+memberchk(Feature,Dead)),Opt).

product_configuration(P):-product_descriptor(_,_,FeatureDict,AttrDict),
						findall(Item,(member(Feature-_,FeatureDict),
										(memberchk(Feature-ValDict,AttrDict)->findall(Key-_,member(Key-_,ValDict),Keys),Item=Feature-Keys;Item=Feature)),P).
