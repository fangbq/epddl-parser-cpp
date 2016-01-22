#include "initial.h"
#include "reader.h"

Reader reader;

Initial::Initial(const char* dFile, const char* pFile) {
    reader.exec(dFile, pFile);

    atomsGrounding();
    episActionsGrounding();
    //onticActionsGrounding();

    init = getEpisDNFfromTree(&reader.init);
    goal = getEpisCNFfromTree(&reader.goal);

    string endFile = "../output/";
    endFile += reader.domainName; endFile += "_initial";

    ofstream out_end(endFile);  // 打开要写入的文本文件
    if(!out_end.is_open()) {
        cout << "cannot open " << endFile << endl;
        return;
    }
    printAtoms(out_end);
    printInit(out_end);
    printGoal(out_end);
    //printSenseActions(out_end);
    //printOnticActions(out_end);  // conversion after first printActions()

    out_end.close();

}

void Initial::printInit(ofstream & out) {
    // print init
    out << "---------------- init -------------------\n";
    init.show(out);
    out << "---------------- done -------------------\n\n";

}

void Initial::printGoal(ofstream & out) {
    // print goal
    out << "---------------- goal -------------------\n";
    goal.show(out);
    out << "---------------- done -------------------\n\n";
}

void Initial::atomsGrounding() {
	/* get atomic propositions from reader */

	// propositions table 
	size_t atomicCounter = 0;
	for (set<string>::iterator it = reader.atomicPropSet.begin();
			it != reader.atomicPropSet.end(); ++it) {
		// cout << *it << endl;
		atomsByName.insert( pair<string, int>(*it, atomicCounter) );
		atomsByIndex.insert( pair<int, string>((atomicCounter++), *it) );
	}

	// predicate grounding
	vector<queue<string> > atoms_vector;
	for (PredicateSet::const_iterator mul_type_pair = reader.predicates.begin();
			mul_type_pair != reader.predicates.end(); ++mul_type_pair) {
		queue<string> tmp_atoms;
		tmp_atoms.push((*mul_type_pair).first);  // grounding atomic proposition
		for (MultiTypeSet::const_iterator sig_type_pair = (*mul_type_pair).second.begin();
				sig_type_pair != (*mul_type_pair).second.end(); ++sig_type_pair) {
			// find objects to the specific type
			for (MultiTypeSet::const_iterator obj = reader.objects.begin();
					obj != reader.objects.end(); ++obj) {
				if((*sig_type_pair).first == (*obj).first) {
					for (size_t i = 0; i < (*sig_type_pair).second.size(); ++i) {
						size_t tmp_atoms_size = tmp_atoms.size();
						for (size_t i = 0; i < tmp_atoms_size; ++i) {
							for (StringSet::const_iterator ob = (*obj).second.begin();
								ob != (*obj).second.end(); ++ob) {
								if (tmp_atoms.front().find(" "+*ob) != string::npos) {
									continue;  // avoid repeat
								}
								// cout << tmp_atoms.front()+" "+*ob << " //test" << endl;
								tmp_atoms.push(tmp_atoms.front()+" "+*ob);
							}
							tmp_atoms.pop();
						}
					}
                    break;
				}
			}
		}
		atoms_vector.push_back(tmp_atoms);
	}

    // pass atoms to map
	for (vector<queue<string> > ::iterator atom_queue = atoms_vector.begin();
		atom_queue != atoms_vector.end(); ++atom_queue) {
		while (!(*atom_queue).empty()) {
			atomsByName.insert( pair<string, int>((*atom_queue).front(), atomicCounter) );
			atomsByIndex.insert( pair<int, string>((atomicCounter++), (*atom_queue).front()) );
			(*atom_queue).pop();
		}
	}
    
}

void Initial::printAtoms(ofstream & out) {
	out << "------------ print all atoms ----------------\n";
	for (int i = 0; i < atomsByIndex.size(); ++i)
	{
		out << i << " : " << atomsByIndex[i] << endl;
	}
	out << "-------- end print all atoms ----------------\n\n";
}

void Initial::episActionsGrounding() {
    for (PreSenseActionList::const_iterator senseAction = reader.senseActions.begin();
        senseAction != reader.senseActions.end(); ++senseAction) {
        queue<PreSenseAction> actions;
        actions.push(*senseAction);
        for(MultiTypeSet::const_iterator param = (*senseAction).paras.begin();
            param != (*senseAction).paras.end(); ++param) {
            // find objects to the specific type of a parameter
            for (MultiTypeSet::const_iterator obj = reader.objects.begin();
                    obj != reader.objects.end(); ++obj) {
                if((*param).first == (*obj).first) {
                    for (size_t i = 0; i < actions.size(); ++i) {
                        for (StringSet::const_iterator ob = (*obj).second.begin();
                            ob != (*obj).second.end(); ++ob) {
                            actions.push(episActionParamGrouding(actions.front(),
                                    *(*param).second.begin(), *ob));
                        }
                        actions.pop();
                    }
                    break;
                }
            }
        }

        // PreSenseActions(reader) to EpisActions(dest structure)
        for(queue<PreSenseAction>::const_iterator action = actions.begin();
            action != actions.end(); ++action) {
            EpisAction epis_action;
            epis_action.name = (*action).name;
            epis_action.pre_con = getEpisCNFfromTree(&(action->preCondition));
        }
    }
}

PreSenseAction Initial::episActionParamGrouding(PreSenseAction & senseAction,
    const string param, const string obj) {
    PreSenseAction action;
    action.name = senseAction.name + obj;
    action.type = senseAction.type;
    action.paras = senseAction.paras;
    action.preCondition = *copyFormula(&senseAction.preCondition);
    action.observe = *copyFormula(&senseAction.observe);
    // precondition
    replaceParamWithObj(&action.preCondition, param, obj);
    // observe
    replaceParamWithObj(&action.observe, param, obj);
    return action;
}

void Initial::replaceParamWithObj(Formula * f, const string param, const string obj) {
    if (f->label == param) f->label = obj;
    if (f->left != NULL)
    {
        replaceParamWithObj(f->left, param, obj);
    }
    if (f->right != NULL)
    {
        replaceParamWithObj(f->right, param, obj);
    }
}

void Initial::printEpisActions(ofstream & out) {

}

void Initial::onticActionsGrounding() {

}

void Initial::printOnticActions(ofstream & out) {

}

EpisDNF Initial::getEpisDNFfromTree(Formula * f) {
    EpisDNF e_dnf;
    stack<Formula*> s;
    do {
        while(f->label != "NULL") {
            if (f->label != "|") {
                e_dnf.epis_terms.push_back(getEpisTermFromTree(f));
                f->label = "NULL";
            } else {
                s.push(f);
                f = f->left;
            }
        }
        if(!s.empty()) {
            f = s.top();
            s.pop();
            f = f->right;
        }
    } while(f->label != "NULL" || !s.empty());
    return e_dnf;
}

EpisTerm Initial::getEpisTermFromTree(Formula * f) {
    EpisTerm e_term;
    stack<Formula*> s;
    do {
        while(f->label != "NULL") {
            if (f->label == "K") {
                e_term.pos_propDNF = e_term.pos_propDNF.group(getPropDNFfromTree(f));
                f->label = "NULL";
            } else if (f->label == "DK") {
                e_term.neg_propDNFs.push_back(getPropDNFfromTree(f));
                f->label = "NULL";
            } else {
                s.push(f);
                f = f->left;
            }
        }
        if(!s.empty()) {
            f = s.top();
            s.pop();
            f = f->right;
        }
    } while(f->label != "NULL" || !s.empty());
    return e_term;
}

PropDNF Initial::getPropDNFfromTree(Formula * f) {
    PropDNF p_dnf;
    stack<Formula*> s;
    do {
        while(f->label != "NULL") {
            if (f->label != "K" && f->label != "DK"
                && f->label != "|") {
                p_dnf.prop_terms.push_back(getPropTermFromTree(f));
                f->label = "NULL";
            } else {
                s.push(f);
                f = f->left;
            }
        }
        if(s.size() > 1) {
            f = s.top();
            s.pop();
            f = f->right;
        }

    } while(f->label != "NULL" || s.size() > 1);
    return p_dnf;
}

PropTerm Initial::getPropTermFromTree(Formula * f) {
    PropTerm p_term(atomsByIndex.size()*2);
    stack<Formula*> s;
    do {
        while(f->label != "NULL") {
            if (f->label != "&") {
                if (f->label == "!") {
                    p_term.literals[atomsByName[f->left->label]*2+1] = 1;
                } else {
                    p_term.literals[atomsByName[f->label]*2] = 1;
                }
                f->label = "NULL";
            } else {
                s.push(f);
                f = f->left;
            }
        }
        if(!s.empty()) {
            f = s.top();
            s.pop();
            f = f->right;
        }
    } while(f->label != "NULL" || !s.empty());
    return p_term;
}

EpisCNF Initial::getEpisCNFfromTree(Formula * f) {
    EpisCNF e_cnf;
    stack<Formula*> s;
    do {
        while(f->label != "NULL") {
            if (f->label != "&") {
                e_cnf.epis_clauses.push_back(getEpisClauseFromTree(f));
                f->label = "NULL";
            } else {
                s.push(f);
                f = f->left;
            }
        }
        if(!s.empty()) {
            f = s.top();
            s.pop();
            f = f->right;
        }
    } while(f->label != "NULL" || !s.empty());
    return e_cnf;
}

EpisClause Initial::getEpisClauseFromTree(Formula * f) {
    EpisClause e_clause;
    stack<Formula*> s;
    do {
        while(f->label != "NULL") {
            if (f->label == "DK") {
                e_clause.neg_propCNF = e_clause.neg_propCNF.group(getPropCNFfromTree(f));
                f->label = "NULL";
            } else if (f->label == "K") {
                e_clause.pos_propCNFs.push_back(getPropCNFfromTree(f));
                f->label = "NULL";
            } else {
                s.push(f);
                f = f->left;
            }
        }
        if(!s.empty()) {
            f = s.top();
            s.pop();
            f = f->right;
        }
    } while(f->label != "NULL" || !s.empty());
    return e_clause;
}

PropCNF Initial::getPropCNFfromTree(Formula * f) {
    PropCNF p_cnf;
    stack<Formula*> s;
    do {
        while(f->label != "NULL") {
            if (f->label != "K" && f->label != "DK"
                && f->label != "&") {
                p_cnf.prop_clauses.push_back(getPropClauseFromTree(f));
                f->label = "NULL";
            } else {
                s.push(f);
                f = f->left;
            }
        }
        if(s.size() > 1) {
            f = s.top();
            s.pop();
            f = f->right;
        }

    } while(f->label != "NULL" || s.size() > 1);
    return p_cnf;
}

PropClause Initial::getPropClauseFromTree(Formula * f) {
    PropClause p_clause(atomsByIndex.size()*2);
    stack<Formula*> s;
    do {
        while(f->label != "NULL") {
            if (f->label != "|") {
                if (f->label == "!") {
                    p_clause.literals[atomsByName[f->left->label]*2+1] = 1;
                } else {
                    p_clause.literals[atomsByName[f->label]*2] = 1;
                }
                f->label = "NULL";
            } else {
                s.push(f);
                f = f->left;
            }
        }
        if(!s.empty()) {
            f = s.top();
            s.pop();
            f = f->right;
        }
    } while(f->label != "NULL" || !s.empty());
    return p_clause;
}
